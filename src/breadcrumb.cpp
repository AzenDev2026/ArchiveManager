#include "breadcrumb.h"

#include <QCompleter>
#include <QEvent>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QShortcut>
#include <QToolButton>

#include "theme.h"

namespace {
//: Below this width the breadcrumb collapses to root › … › last crumbs.
constexpr int COMPACT_WIDTH = 430;
}

Breadcrumb::Breadcrumb(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("breadcrumb"));
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(2, 0, 2, 0);
    m_layout->setSpacing(0);

    m_edit = new QLineEdit(this);
    m_edit->setObjectName(QStringLiteral("address"));
    m_edit->setPlaceholderText(QStringLiteral("Type a path… e.g. /home/you/Documents"));
    m_edit->setClearButtonEnabled(true);
    m_edit->hide();
    m_edit->installEventFilter(this);
    connect(m_edit, &QLineEdit::returnPressed, this, [this] { leaveEditMode(true); });

    // Folder autocomplete while typing an address.
    auto *fsModel = new QFileSystemModel(this);
    fsModel->setRootPath(QStringLiteral("/"));
    fsModel->setFilter(QDir::AllDirs | QDir::Dirs | QDir::NoDotAndDotDot);
    auto *completer = new QCompleter(fsModel, this);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    m_edit->setCompleter(completer);

    auto *escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), m_edit);
    escapeShortcut->setContext(Qt::WidgetShortcut);
    connect(escapeShortcut, &QShortcut::activated, this, [this] { leaveEditMode(false); });
}

void Breadcrumb::setUrl(const QUrl &url)
{
    m_url = url;
    rebuild();
}

void Breadcrumb::rebuild()
{
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }

    struct Crumb {
        QString text;
        QIcon icon;
        QUrl url;
    };
    QList<Crumb> crumbs;
    if (m_url.isLocalFile()) {
        const QString path = m_url.toLocalFile();
        crumbs.append({QStringLiteral("/"),
                       Theme::icon(QStringLiteral("computer"), QStringLiteral("drive-harddisk")),
                       QUrl::fromLocalFile(QStringLiteral("/"))});
        QString cumulative;
        const QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        for (const QString &part : parts) {
            cumulative += QLatin1Char('/') + part;
            crumbs.append({part, QIcon(), QUrl::fromLocalFile(cumulative)});
        }
    } else if (!m_url.isEmpty()) {
        QString title = m_url.toDisplayString(QUrl::PreferLocalFile);
        if (!m_url.scheme().isEmpty()) {
            QString scheme = m_url.scheme();
            scheme[0] = scheme.at(0).toUpper();
            title = scheme;
        }
        crumbs.append({title, Theme::icon(QStringLiteral("folder-remote")), m_url});
    }

    auto addCrumb = [this](const Crumb &crumb) {
        auto *button = new QToolButton(this);
        button->setObjectName(QStringLiteral("crumb"));
        button->setText(crumb.text);
        if (!crumb.icon.isNull()) {
            button->setIcon(crumb.icon);
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        }
        button->setToolTip(crumb.url.toDisplayString(QUrl::PreferLocalFile));
        connect(button, &QToolButton::clicked, this, [this, url = crumb.url] { emit navigateRequested(url); });
        m_layout->addWidget(button);
    };
    auto addSeparator = [this] {
        auto *sep = new QLabel(QStringLiteral("›"), this);
        sep->setObjectName(QStringLiteral("crumbSep"));
        QFont f = sep->font();
        f.setBold(true);
        f.setPixelSize(14);
        sep->setFont(f);
        m_layout->addWidget(sep);
    };
    auto addEllipsis = [this] {
        auto *sep = new QLabel(QStringLiteral("…"), this);
        sep->setObjectName(QStringLiteral("crumbSep"));
        QFont f = sep->font();
        f.setBold(true);
        f.setPixelSize(14);
        sep->setFont(f);
        sep->setToolTip(QStringLiteral("Resize the window to see every folder"));
        m_layout->addWidget(sep);
    };

    // Narrow windows collapse long paths to root › … › last two folders,
    // so the crumbs never crowd or clip the other controls.
    if (m_compact && crumbs.size() > 3) {
        addCrumb(crumbs.first());
        addSeparator();
        addEllipsis();
        addSeparator();
        for (int i = crumbs.size() - 2; i < crumbs.size(); ++i) {
            addCrumb(crumbs.at(i));
        }
    } else {
        for (int i = 0; i < crumbs.size(); ++i) {
            if (i > 0) {
                addSeparator();
            }
            addCrumb(crumbs.at(i));
        }
    }

    m_layout->addStretch(1);
}

void Breadcrumb::enterEditMode()
{
    m_edit->setText(m_url.toDisplayString(QUrl::PreferLocalFile));
    m_edit->setGeometry(rect());
    m_edit->show();
    m_edit->raise();
    m_edit->setFocus();
    m_edit->selectAll();
}

void Breadcrumb::leaveEditMode(bool accept)
{
    if (!m_edit->isVisible()) {
        return;
    }
    m_edit->hide();
    if (accept) {
        const QString text = m_edit->text().trimmed();
        if (!text.isEmpty()) {
            emit textSubmitted(text);
        }
    }
    rebuild();
}

void Breadcrumb::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_edit->isVisible()) {
        enterEditMode();
        return;
    }
    QWidget::mousePressEvent(event);
}

void Breadcrumb::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_edit->setGeometry(rect());
    const bool compact = event->size().width() < COMPACT_WIDTH;
    if (compact != m_compact) {
        m_compact = compact;
        rebuild();
    }
}

bool Breadcrumb::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_edit && event->type() == QEvent::FocusOut) {
        leaveEditMode(false);
    }
    return QWidget::eventFilter(watched, event);
}
