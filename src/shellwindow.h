#pragma once

#include <QList>
#include <QMainWindow>
#include <QUrl>

#include "theme.h"

class QAction;
class QCloseEvent;
class QLabel;
class QLineEdit;
class QSplitter;
class QStackedWidget;
class QTabBar;
class QToolButton;
class Breadcrumb;
class Sidebar;
class WindowControls;

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
    void dumpActionsAndQuit();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    struct Tab {
        KParts::ReadOnlyPart *part = nullptr;
        QWidget *host = nullptr;
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
    void closeTab(int index);
    void switchToTab(int index);
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

    void applyTheme(Theme::Mode mode);
    void toggleTheme();
    void openInDolphin();
    void showAbout();
    void openItemUrl(const QUrl &url);

    QWidget *m_root = nullptr;
    QWidget *m_titleBar = nullptr;
    QWidget *m_commandBar = nullptr;
    QWidget *m_statusBar = nullptr;
    QTabBar *m_tabs = nullptr;
    QToolButton *m_addTabButton = nullptr;
    WindowControls *m_controls = nullptr;

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

    Breadcrumb *m_breadcrumb = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QSplitter *m_splitter = nullptr;
    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_stack = nullptr;
    QLabel *m_statusLabel = nullptr;
    QToolButton *m_themeButton = nullptr;

    QList<Tab *> m_tabsData;
    Theme::Mode m_mode = Theme::Mode::Dark;
};
