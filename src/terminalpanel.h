#pragma once

#include <QWidget>

class QLabel;
class QToolButton;

namespace KParts {
class ReadOnlyPart;
}

// Bottom terminal panel embedding Konsole's KPart, so a shell is always
// one keystroke away without leaving the window.
class TerminalPanel : public QWidget
{
    Q_OBJECT
public:
    explicit TerminalPanel(QWidget *parent = nullptr);

    bool isReady() const;

    // Change the shell's working directory ("cd here").
    void cdTo(const QString &dir);

    // Type a command (and a newline) into the shell.
    void runCommand(const QString &command);

signals:
    void hideRequested();
    void cdRequested();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void ensurePart();
    void sendInput(const QString &text);
    static QString shellQuote(const QString &text);

    KParts::ReadOnlyPart *m_part = nullptr;
    QWidget *m_header = nullptr;
    QLabel *m_titleLabel = nullptr;
    QToolButton *m_cdButton = nullptr;
    QToolButton *m_closeButton = nullptr;
    QString m_pendingDir;
    bool m_partTried = false;
};
