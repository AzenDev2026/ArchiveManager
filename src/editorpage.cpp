#include "editorpage.h"

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QToolButton>
#include <QVBoxLayout>

EditorPage::EditorPage(const QString &filePath, QWidget *parent)
    : QWidget(parent)
    , m_path(filePath)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setupToolbar();
    layout->addWidget(m_toolbar);

    m_doc = KTextEditor::Editor::instance()->createDocument(this);
    m_view = m_doc->createView(this);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu); // keep the editor's own context menu
    layout->addWidget(m_view);

    if (!filePath.isEmpty()) {
        m_doc->openUrl(QUrl::fromLocalFile(filePath));
    }
    connect(m_doc, &KTextEditor::Document::modifiedChanged, this,
            [this](KTextEditor::Document *) { emit modifiedChanged(isModified()); });

    updateRunButton();
    m_view->setFocus();
}

EditorPage::~EditorPage() = default;

QString EditorPage::filePath() const
{
    return m_path;
}

bool EditorPage::isUntitled() const
{
    return m_path.isEmpty();
}

bool EditorPage::isModified() const
{
    return m_doc && m_doc->isModified();
}

bool EditorPage::save()
{
    if (isUntitled()) {
        return saveAs();
    }
    const bool ok = m_doc->documentSave();
    if (ok) {
        emit modifiedChanged(false);
        m_pathLabel->setText(m_path);
    }
    return ok;
}

bool EditorPage::saveAs()
{
    const QString startDir = m_path.isEmpty() ? QDir::homePath() : QFileInfo(m_path).absolutePath();
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Save file"), startDir);
    if (path.isEmpty()) {
        return false;
    }
    m_path = path;
    const bool ok = m_doc->saveAs(QUrl::fromLocalFile(path));
    if (ok) {
        emit modifiedChanged(false);
        emit pathChanged(path);
        m_pathLabel->setText(path);
        updateRunButton();
    }
    return ok;
}

QString EditorPage::title() const
{
    if (isUntitled()) {
        return QStringLiteral("Untitled");
    }
    return QFileInfo(m_path).fileName();
}

QString EditorPage::runCommand() const
{
    if (m_path.isEmpty()) {
        return QString();
    }
    const QFileInfo fi(m_path);
    const QString suffix = fi.suffix().toLower();
    const QString quoted = shellQuote(m_path);
    if (suffix == QLatin1String("sh") || suffix == QLatin1String("bash")) {
        return QStringLiteral("bash ") + quoted;
    }
    if (suffix == QLatin1String("py")) {
        return QStringLiteral("python3 ") + quoted;
    }
    if (suffix == QLatin1String("js")) {
        return QStringLiteral("node ") + quoted;
    }
    if (suffix == QLatin1String("cpp")) {
        return QStringLiteral("g++ ") + quoted + QStringLiteral(" -o /tmp/archive-manager-run && /tmp/archive-manager-run");
    }
    if (suffix == QLatin1String("c")) {
        return QStringLiteral("gcc ") + quoted + QStringLiteral(" -o /tmp/archive-manager-run && /tmp/archive-manager-run");
    }
    return QString();
}

void EditorPage::setupToolbar()
{
    m_toolbar = new QWidget(this);
    m_toolbar->setObjectName(QStringLiteral("editorBar"));
    m_toolbar->setFixedHeight(40);
    auto *barLayout = new QHBoxLayout(m_toolbar);
    barLayout->setContentsMargins(10, 4, 8, 4);
    barLayout->setSpacing(4);

    m_pathLabel = new QLabel(m_toolbar);
    m_pathLabel->setObjectName(QStringLiteral("editorPath"));
    m_pathLabel->setText(m_path.isEmpty() ? QStringLiteral("Untitled") : m_path);
    m_pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    barLayout->addWidget(m_pathLabel, 1);

    m_saveButton = new QToolButton(m_toolbar);
    m_saveButton->setObjectName(QStringLiteral("cmd"));
    m_saveButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_saveButton->setText(QStringLiteral("Save"));
    m_saveButton->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    m_saveButton->setShortcut(QKeySequence::Save);
    connect(m_saveButton, &QToolButton::clicked, this, [this] { save(); });
    barLayout->addWidget(m_saveButton);

    m_runButton = new QToolButton(m_toolbar);
    m_runButton->setObjectName(QStringLiteral("cmd"));
    m_runButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_runButton->setText(QStringLiteral("Run"));
    m_runButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    connect(m_runButton, &QToolButton::clicked, this, &EditorPage::onRunClicked);
    barLayout->addWidget(m_runButton);
}

void EditorPage::updateRunButton()
{
    const QString cmd = runCommand();
    m_runButton->setEnabled(!cmd.isEmpty());
    m_runButton->setToolTip(cmd.isEmpty()
                                ? QStringLiteral("Run is available for .sh, .py, .js, .c and .cpp files")
                                : QStringLiteral("Save and run in the terminal"));
}

void EditorPage::onRunClicked()
{
    const QString cmd = runCommand();
    if (cmd.isEmpty()) {
        return;
    }
    if (!save()) {
        return;
    }
    const QString workDir = QFileInfo(m_path).absolutePath();
    emit requestRun(cmd, workDir);
}

QString EditorPage::shellQuote(const QString &text)
{
    QString quoted = text;
    quoted.replace(QLatin1Char('\''), QLatin1String("'\\''"));
    return QLatin1Char('\'') + quoted + QLatin1Char('\'');
}
