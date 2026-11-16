#include "terminalpanel.h"

#include <kpluginmetadata.h>
#include <KParts/PartLoader>
#include <KParts/ReadOnlyPart>

#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QShowEvent>
#include <QToolButton>
#include <QVBoxLayout>

TerminalPanel::TerminalPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("terminalPanel"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ---- Header ----
    m_header = new QWidget(this);
    m_header->setObjectName(QStringLiteral("terminalHeader"));
    m_header->setFixedHeight(30);
    auto *headerLayout = new QHBoxLayout(m_header);
    headerLayout->setContentsMargins(10, 0, 6, 0);
    headerLayout->setSpacing(6);

    m_titleLabel = new QLabel(QStringLiteral("Terminal"), m_header);
    m_titleLabel->setObjectName(QStringLiteral("terminalTitle"));
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch(1);

    m_cdButton = new QToolButton(m_header);
    m_cdButton->setObjectName(QStringLiteral("statusBtn"));
    m_cdButton->setAutoRaise(true);
    m_cdButton->setText(QStringLiteral("cd here"));
    m_cdButton->setIcon(QIcon::fromTheme(QStringLiteral("go-jump")));
    m_cdButton->setToolTip(QStringLiteral("Switch the terminal to the folder open in the current tab"));
    m_cdButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    headerLayout->addWidget(m_cdButton);

    m_closeButton = new QToolButton(m_header);
    m_closeButton->setObjectName(QStringLiteral("statusBtn"));
    m_closeButton->setAutoRaise(true);
    m_closeButton->setText(QStringLiteral("✕"));
    m_closeButton->setToolTip(QStringLiteral("Hide the terminal (Ctrl+`)"));
    headerLayout->addWidget(m_closeButton);
    connect(m_closeButton, &QToolButton::clicked, this, &TerminalPanel::hideRequested);
    connect(m_cdButton, &QToolButton::clicked, this, &TerminalPanel::cdRequested);

    layout->addWidget(m_header);

    // The Konsole part fills the rest once created (see ensurePart).
}

bool TerminalPanel::isReady() const
{
    return m_part != nullptr;
}

void TerminalPanel::cdTo(const QString &dir)
{
    if (dir.isEmpty()) {
        return;
    }
    m_pendingDir = dir;
    if (!m_part) {
        ensurePart();
        return;
    }
    sendInput(QStringLiteral("cd %1\n").arg(shellQuote(dir)));
}

void TerminalPanel::runCommand(const QString &command)
{
    if (command.isEmpty()) {
        return;
    }
    if (!m_part) {
        ensurePart();
    }
    sendInput(command + QLatin1Char('\n'));
}

void TerminalPanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    ensurePart();
}

void TerminalPanel::ensurePart()
{
    if (m_part || m_partTried) {
        return;
    }
    m_partTried = true;

    const KPluginMetaData meta = KPluginMetaData::findPluginById(QStringLiteral("kf6/parts"), QStringLiteral("konsolepart"));
    if (!meta.isValid()) {
        auto *errorLabel = new QLabel(QStringLiteral("Konsole's terminal engine (konsolepart) was not found."), this);
        errorLabel->setAlignment(Qt::AlignCenter);
        qobject_cast<QVBoxLayout *>(layout())->addWidget(errorLabel, 1);
        return;
    }

    const auto result = KParts::PartLoader::instantiatePart<KParts::ReadOnlyPart>(meta, this, this);
    if (!result) {
        auto *errorLabel = new QLabel(
            QStringLiteral("Could not load Konsole's terminal engine:\n%1").arg(result.errorString), this);
        errorLabel->setAlignment(Qt::AlignCenter);
        errorLabel->setWordWrap(true);
        qobject_cast<QVBoxLayout *>(layout())->addWidget(errorLabel, 1);
        return;
    }

    m_part = result.plugin;
    qobject_cast<QVBoxLayout *>(layout())->addWidget(m_part->widget(), 1);

    // Start the first session in the requested folder (defaults to Home).
    const QString dir = m_pendingDir.isEmpty() ? QDir::homePath() : m_pendingDir;
    if (!QMetaObject::invokeMethod(m_part, "setInitialWorkingDirectory", Qt::DirectConnection, Q_ARG(QString, dir))) {
        qWarning("TerminalPanel: setInitialWorkingDirectory not available on konsolepart");
    }
}

void TerminalPanel::sendInput(const QString &text)
{
    if (!m_part) {
        return;
    }
    if (QMetaObject::invokeMethod(m_part, "sendInput", Qt::DirectConnection, Q_ARG(QString, text))) {
        return;
    }
    // Fallback: run the command through a fresh shell session.
    const QStringList args{QStringLiteral("-c"), text};
    if (!QMetaObject::invokeMethod(m_part, "startProgram", Qt::DirectConnection,
                                   Q_ARG(QString, QStringLiteral("/bin/sh")), Q_ARG(QStringList, args))) {
        qWarning("TerminalPanel: neither sendInput nor startProgram available on konsolepart");
    }
}

QString TerminalPanel::shellQuote(const QString &text)
{
    QString quoted = text;
    quoted.replace(QLatin1Char('\''), QLatin1String("'\\''"));
    return QLatin1Char('\'') + quoted + QLatin1Char('\'');
}
