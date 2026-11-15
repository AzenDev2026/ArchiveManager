#include "breadcrumb.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QShortcut>
#include <QToolButton>

#include "theme.h"

Breadcrumb::Breadcrumb(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("breadcrumb"));
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(2, 0, 2, 0);
    m_layout->setSpacing(0);

    m_edit = new QLineEdit(this);
    m_edit->setObjectName(QStringLiteral("address"));
    m_edit->hide();
    m_edit->installEventFilter(this);
    connect(m_edit, &QLineEdit::returnPressed, this, [this] { leaveEditMode(true); });

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

    auto addCrumb = [this](const QString &text, const QIcon &icon, const QUrl &url) {
        auto *button = new QToolButton(this);
        button->setObjectName(QStringLiteral("crumb"));
        button->setText(text);
        if (!icon.isNull()) {
            button->setIcon(icon);
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        }
        connect(button, &QToolButton::clicked, this, [this, url] { emit navigateRequested(url); });
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

    if (m_url.isLocalFile()) {
        const QString path = m_url.toLocalFile();
        addCrumb(QStringLiteral("/"),
                 Theme::icon(QStringLiteral("computer"), QStringLiteral("drive-harddisk")),
                 QUrl::fromLocalFile(QStringLiteral("/")));
        QString cumulative;
        const QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        for (const QString &part : parts) {
            cumulative += QLatin1Char('/') + part;
            addSeparator();
            addCrumb(part, QIcon(), QUrl::fromLocalFile(cumulative));
        }
    } else if (!m_url.isEmpty()) {
        QString title = m_url.toDisplayString(QUrl::PreferLocalFile);
        if (!m_url.scheme().isEmpty()) {
            QString scheme = m_url.scheme();
            scheme[0] = scheme.at(0).toUpper();
            title = scheme;
        }
        addCrumb(title, Theme::icon(QStringLiteral("folder-remote")), m_url);
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
}

bool Breadcrumb::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_edit && event->type() == QEvent::FocusOut) {
        leaveEditMode(false);
    }
    return QWidget::eventFilter(watched, event);
}
