#pragma once

#include <QWidget>

class QLabel;
class QToolButton;

namespace KTextEditor {
class View;
class Document;
}

// One editor page: a KTextEditor view with a small toolbar (save / run).
class EditorPage : public QWidget
{
    Q_OBJECT
public:
    explicit EditorPage(const QString &filePath, QWidget *parent = nullptr);
    ~EditorPage() override;

    QString filePath() const;
    bool isUntitled() const;
    bool isModified() const;

    bool save();
    bool saveAs();

    QString title() const;

    // Command used by the "Run" button, or empty when the file type has no runner.
    QString runCommand() const;

signals:
    void modifiedChanged(bool modified);
    void pathChanged(const QString &path);
    void requestRun(const QString &command, const QString &workDir);

private:
    void setupToolbar();
    void updateRunButton();
    void onRunClicked();
    static QString shellQuote(const QString &text);

    KTextEditor::Document *m_doc = nullptr;
    KTextEditor::View *m_view = nullptr;
    QWidget *m_toolbar = nullptr;
    QLabel *m_pathLabel = nullptr;
    QToolButton *m_saveButton = nullptr;
    QToolButton *m_runButton = nullptr;
    QString m_path;
};
