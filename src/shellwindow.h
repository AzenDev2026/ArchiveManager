#pragma once

#include <QList>
#include <QMainWindow>
#include <QUrl>

#include "theme.h"

class QAction;
class QCloseEvent;
class QLabel;
class QLineEdit;
class QResizeEvent;
class QSplitter;
class QStackedWidget;
class QTabBar;
class QToolButton;
class Breadcrumb;
class EditorPage;
class Sidebar;
class TerminalPanel;

namespace KParts {
class ReadOnlyPart;
}

class ShellWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ShellWindow(QWidget *parent = nullptr, const QString &forcedTheme = QString());
    ~ShellWindow() override;

    void openInitialUrl(const QUrl &url);
    void openFileInEditor(const QString &filePath);
    void showTerminal();
    void dumpActionsAndQuit();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Tab {
        enum class Kind { File, Editor };
        Kind kind = Kind::File;
        KParts::ReadOnlyPart *part = nullptr;
        QWidget *host = nullptr;
        EditorPage *editor = nullptr;
        QList<QUrl> history;
        int historyPos = -1;
        bool skipRecord = false;
        bool enforceCompact = true;
        bool applyingDefault = false;
    };

    Tab *tabAt(int index);
    Tab *currentTab() const;
    Tab *tabOfPart(KParts::ReadOnlyPart *part) const;
    KParts::ReadOnlyPart *currentPart() const;

    int addTab(const QUrl &initialUrl);
    int addEditorTab(const QString &filePath = QString());
    void closeTab(int index);
    void switchToTab(int index);
    void applyCommandBarForTab(Tab *tab);
    void updateEditorTabText(Tab *tab);
    void connectPartSignals(Tab *tab);
    void rebindToCurrentPart();

    void navigate(const QUrl &url);
    void syncUrl(Tab *tab);
    void recordUrl(Tab *tab, const QUrl &url);
    void updateNavButtons();
    void updateTabText(Tab *tab, const QString &caption);

private Q_SLOTS:
    void goBack();
    void goForward();
    void goUp();

private:
    QAction *partAction(const char *name) const;
    void onSearchSubmitted();

    void rebuildSortMenu();
    void rebuildViewMenu();
    void rebuildMoreMenu();
    void updateCommandBarCompact();

    void applyTheme(Theme::Mode mode);
    void toggleTheme();
    void openInDolphin();
    void showAbout();
    void openItemUrl(const QUrl &url);
    void openInEditor(const QUrl &url);
    void runInTerminal(const QString &command, const QString &workDir);
    void setTerminalVisible(bool visible);
    void cdTerminalHere();

    QWidget *m_root = nullptr;
    QWidget *m_titleBar = nullptr;
    QWidget *m_commandBar = nullptr;
    QWidget *m_statusBar = nullptr;
    QTabBar *m_tabs = nullptr;
    QToolButton *m_addTabButton = nullptr;

    QAction *m_backAction = nullptr;
    QAction *m_forwardAction = nullptr;
    QAction *m_upAction = nullptr;
    QAction *m_cutAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_pasteAction = nullptr;
    QAction *m_themeAction = nullptr;
    QAction *m_sidebarAction = nullptr;

    QToolButton *m_backButton = nullptr;
    QToolButton *m_forwardButton = nullptr;
    QToolButton *m_upButton = nullptr;
    QToolButton *m_newButton = nullptr;
    QToolButton *m_cutButton = nullptr;
    QToolButton *m_copyButton = nullptr;
    QToolButton *m_pasteButton = nullptr;
    QToolButton *m_renameButton = nullptr;
    QToolButton *m_deleteButton = nullptr;
    QToolButton *m_sortButton = nullptr;
    QToolButton *m_viewButton = nullptr;
    QToolButton *m_moreButton = nullptr;
    QToolButton *m_editorButton = nullptr;
    QToolButton *m_terminalButton = nullptr;

    Breadcrumb *m_breadcrumb = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QSplitter *m_splitter = nullptr;
    QSplitter *m_verticalSplitter = nullptr;
    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_stack = nullptr;
    QLabel *m_statusLabel = nullptr;
    QToolButton *m_themeButton = nullptr;
    TerminalPanel *m_terminal = nullptr;

    QList<QWidget *> m_fileCommandWidgets;
    QList<QToolButton *> m_textCommandButtons;
    QList<Tab *> m_tabsData;
    Theme::Mode m_mode = Theme::Mode::Dark;
    bool m_compact = false;
};
