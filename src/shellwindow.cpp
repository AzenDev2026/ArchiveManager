#include "shellwindow.h"

#include "breadcrumb.h"
#include "sidebar.h"
#include "windowcontrols.h"

#include <kactioncollection.h>
#include <kfileitem.h>
#include <kpluginmetadata.h>
#include <KParts/NavigationExtension>
#include <KParts/PartLoader>
#include <KParts/ReadOnlyPart>

#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProcess>
#include <QSet>
#include <QSettings>
#include <QShortcut>
#include <QSizeGrip>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QWindow>
#include <utility>

ShellWindow::ShellWindow(QWidget *parent, const QString &forcedTheme)
    : QMainWindow(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlag(Qt::FramelessWindowHint);
    setWindowTitle(QStringLiteral("Dolphin Shell"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/dolphin-shell.svg")));
    setMinimumSize(900, 560);
    resize(1240, 760);

    if (forcedTheme == QLatin1String("light")) {
        m_mode = Theme::Mode::Light;
    } else if (forcedTheme == QLatin1String("dark")) {
        m_mode = Theme::Mode::Dark;
    } else {
        m_mode = Theme::loadMode();
    }
    applyTheme(m_mode);

    m_root = new QWidget(this);
    m_root->setObjectName(QStringLiteral("root"));
    m_root->installEventFilter(this);
    setCentralWidget(m_root);

    auto *rootLayout = new QVBoxLayout(m_root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- Title bar ----
    m_titleBar = new QWidget(m_root);
    m_titleBar->setObjectName(QStringLiteral("titleBar"));
    m_titleBar->setFixedHeight(40);
    m_titleBar->installEventFilter(this);
    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(10, 0, 0, 0);
    titleLayout->setSpacing(6);

    auto *appIcon = new QLabel(m_titleBar);
    appIcon->setPixmap(QIcon(QStringLiteral(":/icons/dolphin-shell.svg")).pixmap(20, 20));
    titleLayout->addWidget(appIcon);

    m_tabs = new QTabBar(m_titleBar);
    m_tabs->setExpanding(false);
    m_tabs->setMovable(true);
    m_tabs->setTabsClosable(true);
    m_tabs->setDrawBase(false);
    m_tabs->setDocumentMode(true);
    m_tabs->setUsesScrollButtons(true);
    m_tabs->setElideMode(Qt::ElideRight);
    m_tabs->installEventFilter(this);
    titleLayout->addWidget(m_tabs, 1);
    connect(m_tabs, &QTabBar::tabCloseRequested, this, &ShellWindow::closeTab);
    connect(m_tabs, &QTabBar::currentChanged, this, &ShellWindow::switchToTab);

    m_addTabButton = new QToolButton(m_titleBar);
    m_addTabButton->setObjectName(QStringLiteral("tabAdd"));
    m_addTabButton->setText(QStringLiteral("+"));
    m_addTabButton->setToolTip(QStringLiteral("New tab (Ctrl+T)"));
    m_addTabButton->setAutoRaise(true);
    titleLayout->addWidget(m_addTabButton);
    connect(m_addTabButton, &QToolButton::clicked, this, [this] {
        Tab *tab = currentTab();
        addTab(tab && tab->part ? tab->part->url() : QUrl());
    });

    m_controls = new WindowControls(m_titleBar);
    titleLayout->addWidget(m_controls);
    connect(m_controls, &WindowControls::clicked, this, [this](int button) {
        switch (button) {
        case WindowControls::Minimize:
            showMinimized();
            break;
        case WindowControls::Maximize:
            setWindowState(isMaximized() ? Qt::WindowNoState : Qt::WindowMaximized);
            break;
        case WindowControls::Close:
            close();
            break;
        }
    });

    rootLayout->addWidget(m_titleBar);

    // ---- Command bar ----
    m_commandBar = new QWidget(m_root);
    m_commandBar->setObjectName(QStringLiteral("commandBar"));
    m_commandBar->setFixedHeight(44);
    m_commandBar->installEventFilter(this);
    auto *commandLayout = new QHBoxLayout(m_commandBar);
    commandLayout->setContentsMargins(10, 6, 8, 6);
    commandLayout->setSpacing(2);

    QIcon backIcon = Theme::icon(QStringLiteral("go-previous"), QStringLiteral("arrow-left"));
    if (backIcon.isNull()) {
        backIcon = style()->standardIcon(QStyle::SP_ArrowBack);
    }
    m_backAction = new QAction(backIcon, QStringLiteral("Back"), this);
    m_backAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
    connect(m_backAction, &QAction::triggered, this, &ShellWindow::goBack);
    m_backButton = new QToolButton(m_commandBar);
    m_backButton->setObjectName(QStringLiteral("nav"));
    m_backButton->setDefaultAction(m_backAction);
    m_backButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    commandLayout->addWidget(m_backButton);

    QIcon forwardIcon = Theme::icon(QStringLiteral("go-next"), QStringLiteral("arrow-right"));
    if (forwardIcon.isNull()) {
        forwardIcon = style()->standardIcon(QStyle::SP_ArrowForward);
    }
    m_forwardAction = new QAction(forwardIcon, QStringLiteral("Forward"), this);
    m_forwardAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Right));
    connect(m_forwardAction, &QAction::triggered, this, &ShellWindow::goForward);
    m_forwardButton = new QToolButton(m_commandBar);
    m_forwardButton->setObjectName(QStringLiteral("nav"));
    m_forwardButton->setDefaultAction(m_forwardAction);
    m_forwardButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    commandLayout->addWidget(m_forwardButton);

    QIcon upIcon = Theme::icon(QStringLiteral("go-up"), QStringLiteral("arrow-up"));
    if (upIcon.isNull()) {
        upIcon = style()->standardIcon(QStyle::SP_ArrowUp);
    }
    m_upAction = new QAction(upIcon, QStringLiteral("Up"), this);
    m_upAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Up));
    connect(m_upAction, &QAction::triggered, this, &ShellWindow::goUp);
    m_upButton = new QToolButton(m_commandBar);
    m_upButton->setObjectName(QStringLiteral("nav"));
    m_upButton->setDefaultAction(m_upAction);
    m_upButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    commandLayout->addWidget(m_upButton);

    m_breadcrumb = new Breadcrumb(m_commandBar);
    commandLayout->addWidget(m_breadcrumb, 1);
    connect(m_breadcrumb, &Breadcrumb::navigateRequested, this, &ShellWindow::navigate);
    connect(m_breadcrumb, &Breadcrumb::textSubmitted, this, [this](const QString &text) {
        navigate(QUrl::fromUserInput(text, QDir::homePath()));
    });

    m_cutAction = new QAction(Theme::icon(QStringLiteral("edit-cut")), QStringLiteral("Cut"), this);
    m_cutAction->setShortcut(QKeySequence::Cut);
    connect(m_cutAction, &QAction::triggered, this, [this] {
        if (KParts::ReadOnlyPart *part = currentPart()) {
            QMetaObject::invokeMethod(part->widget(), "cutSelectedItemsToClipboard", Qt::DirectConnection);
        }
    });
    m_copyAction = new QAction(Theme::icon(QStringLiteral("edit-copy")), QStringLiteral("Copy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    connect(m_copyAction, &QAction::triggered, this, [this] {
        if (KParts::ReadOnlyPart *part = currentPart()) {
            QMetaObject::invokeMethod(part->widget(), "copySelectedItemsToClipboard", Qt::DirectConnection);
        }
    });
    m_pasteAction = new QAction(Theme::icon(QStringLiteral("edit-paste")), QStringLiteral("Paste"), this);
    m_pasteAction->setShortcut(QKeySequence::Paste);
    connect(m_pasteAction, &QAction::triggered, this, [this] {
        if (KParts::ReadOnlyPart *part = currentPart()) {
            QMetaObject::invokeMethod(part->widget(), "paste", Qt::DirectConnection);
        }
    });
    m_cutAction->setEnabled(false);
    m_copyAction->setEnabled(false);
    m_pasteAction->setEnabled(false);

    m_searchEdit = new QLineEdit(m_commandBar);
    m_searchEdit->setObjectName(QStringLiteral("search"));
    m_searchEdit->setPlaceholderText(QStringLiteral("Search in this folder"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedWidth(220);
    const QIcon searchIcon = Theme::icon(QStringLiteral("edit-find"), QStringLiteral("system-search"));
    if (!searchIcon.isNull()) {
        m_searchEdit->addAction(searchIcon, QLineEdit::LeadingPosition);
    }
    commandLayout->addWidget(m_searchEdit);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &ShellWindow::onSearchSubmitted);

    commandLayout->addSpacing(4);

    auto makeCommandButton = [&] {
        auto *button = new QToolButton(m_commandBar);
        button->setObjectName(QStringLiteral("cmd"));
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setIconSize(QSize(16, 16));
        commandLayout->addWidget(button);
        return button;
    };
    m_newButton = makeCommandButton();
    m_cutButton = makeCommandButton();
    m_copyButton = makeCommandButton();
    m_pasteButton = makeCommandButton();
    m_renameButton = makeCommandButton();
    m_deleteButton = makeCommandButton();
    m_sortButton = makeCommandButton();
    m_viewButton = makeCommandButton();
    m_moreButton = makeCommandButton();

    rootLayout->addWidget(m_commandBar);

    // ---- Splitter (sidebar + tabs) ----
    m_splitter = new QSplitter(Qt::Horizontal, m_root);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(1);
    m_splitter->installEventFilter(this);

    m_sidebar = new Sidebar(m_splitter);
    m_sidebar->setFixedWidth(232);
    connect(m_sidebar, &Sidebar::navigateRequested, this, &ShellWindow::navigate);

    m_stack = new QStackedWidget(m_splitter);
    m_stack->setObjectName(QStringLiteral("stack"));
    m_splitter->addWidget(m_sidebar);
    m_splitter->addWidget(m_stack);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({232, 1000});
    rootLayout->addWidget(m_splitter, 1);

    // ---- Status bar ----
    m_statusBar = new QWidget(m_root);
    m_statusBar->setObjectName(QStringLiteral("statusBar"));
    m_statusBar->setFixedHeight(26);
    m_statusBar->installEventFilter(this);
    auto *statusLayout = new QHBoxLayout(m_statusBar);
    statusLayout->setContentsMargins(12, 0, 6, 0);
    statusLayout->setSpacing(6);

    m_statusLabel = new QLabel(m_statusBar);
    m_statusLabel->setObjectName(QStringLiteral("statusLabel"));
    m_statusLabel->setTextFormat(Qt::AutoText);
    statusLayout->addWidget(m_statusLabel, 1);

    m_themeButton = new QToolButton(m_statusBar);
    m_themeButton->setObjectName(QStringLiteral("statusBtn"));
    m_themeButton->setAutoRaise(true);
    statusLayout->addWidget(m_themeButton);
    connect(m_themeButton, &QToolButton::clicked, this, &ShellWindow::toggleTheme);

    auto *sizeGrip = new QSizeGrip(m_statusBar);
    statusLayout->addWidget(sizeGrip, 0, Qt::AlignBottom | Qt::AlignRight);

    rootLayout->addWidget(m_statusBar);

    // ---- Sidebar and theme actions ----
    m_sidebarAction = new QAction(QStringLiteral("Navigation pane"), this);
    m_sidebarAction->setCheckable(true);
    connect(m_sidebarAction, &QAction::triggered, this, [this](bool checked) {
        m_sidebar->setVisible(checked);
        QSettings().setValue(QStringLiteral("sidebarVisible"), checked);
    });
    connect(m_sidebar, &Sidebar::collapseRequested, this, [this] {
        m_sidebar->setVisible(false);
        m_sidebarAction->setChecked(false);
        QSettings().setValue(QStringLiteral("sidebarVisible"), false);
    });

    m_themeAction = new QAction(QStringLiteral("Dark mode"), this);
    m_themeAction->setCheckable(true);
    connect(m_themeAction, &QAction::triggered, this, &ShellWindow::toggleTheme);

    // ---- Shortcuts ----
    auto *newTabShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+T")), this);
    connect(newTabShortcut, &QShortcut::activated, this, [this] {
        Tab *tab = currentTab();
        addTab(tab && tab->part ? tab->part->url() : QUrl());
    });
    auto *closeTabShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+W")), this);
    connect(closeTabShortcut, &QShortcut::activated, this, [this] { closeTab(m_tabs->currentIndex()); });
    auto *nextTabShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Tab")), this);
    connect(nextTabShortcut, &QShortcut::activated, this, [this] {
        if (m_tabs->count() > 1) {
            m_tabs->setCurrentIndex((m_tabs->currentIndex() + 1) % m_tabs->count());
        }
    });
    auto *prevTabShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+Tab")), this);
    connect(prevTabShortcut, &QShortcut::activated, this, [this] {
        if (m_tabs->count() > 1) {
            m_tabs->setCurrentIndex((m_tabs->currentIndex() - 1 + m_tabs->count()) % m_tabs->count());
        }
    });
    auto *addressShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+L")), this);
    connect(addressShortcut, &QShortcut::activated, this, [this] { m_breadcrumb->enterEditMode(); });
    auto *searchShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+F")), this);
    connect(searchShortcut, &QShortcut::activated, this, [this] {
        m_searchEdit->setFocus();
        m_searchEdit->selectAll();
    });

    // ---- Settings ----
    QSettings settings;
    restoreGeometry(settings.value(QStringLiteral("geometry")).toByteArray());
    const bool sidebarVisible = settings.value(QStringLiteral("sidebarVisible"), true).toBool();
    m_sidebar->setVisible(sidebarVisible);
    m_sidebarAction->setChecked(sidebarVisible);

    // ---- First tab ----
    addTab(QUrl());
    rebindToCurrentPart();
    applyTheme(m_mode);
}

ShellWindow::~ShellWindow() = default;

ShellWindow::Tab *ShellWindow::tabAt(int index)
{
    if (index < 0 || index >= m_tabs->count()) {
        return nullptr;
    }
    const QVariant data = m_tabs->tabData(index);
    if (!data.isValid()) {
        return nullptr;
    }
    return reinterpret_cast<Tab *>(data.value<qulonglong>());
}

ShellWindow::Tab *ShellWindow::currentTab() const
{
    const int index = m_tabs->currentIndex();
    if (index < 0) {
        return nullptr;
    }
    const QVariant data = m_tabs->tabData(index);
    if (!data.isValid()) {
        return nullptr;
    }
    return reinterpret_cast<Tab *>(data.value<qulonglong>());
}

ShellWindow::Tab *ShellWindow::tabOfPart(KParts::ReadOnlyPart *part) const
{
    for (Tab *tab : m_tabsData) {
        if (tab->part == part) {
            return tab;
        }
    }
    return nullptr;
}

KParts::ReadOnlyPart *ShellWindow::currentPart() const
{
    Tab *tab = currentTab();
    return tab ? tab->part : nullptr;
}

int ShellWindow::addTab(const QUrl &initialUrl)
{
    auto *tab = new Tab;
    tab->host = new QWidget(m_stack);
    tab->host->setObjectName(QStringLiteral("hostPage"));
    auto *hostLayout = new QVBoxLayout(tab->host);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->setSpacing(0);

    QString error;
    const auto partResult = KParts::PartLoader::instantiatePartForMimeType<KParts::ReadOnlyPart>(
        QStringLiteral("inode/directory"), tab->host, this);
    if (partResult) {
        tab->part = partResult.plugin;
    } else {
        error = partResult.errorString;
        const KPluginMetaData meta =
            KPluginMetaData::findPluginById(QStringLiteral("kf6/parts"), QStringLiteral("dolphinpart"));
        if (meta.isValid()) {
            const auto fallback = KParts::PartLoader::instantiatePart<KParts::ReadOnlyPart>(meta, tab->host, this);
            if (fallback) {
                tab->part = fallback.plugin;
            } else {
                error = fallback.errorString;
            }
        }
    }

    const int index = m_tabs->addTab(QStringLiteral("Home"));
    m_tabs->setTabData(index, QVariant::fromValue(static_cast<qulonglong>(reinterpret_cast<quintptr>(tab))));
    m_tabsData.append(tab);
    m_stack->addWidget(tab->host);

    if (!tab->part) {
        auto *errorLabel = new QLabel(
            QStringLiteral("Could not load Dolphin's engine (dolphinpart).\n%1").arg(error), tab->host);
        errorLabel->setAlignment(Qt::AlignCenter);
        errorLabel->setWordWrap(true);
        hostLayout->addWidget(errorLabel);
    } else {
        hostLayout->addWidget(tab->part->widget());
        connectPartSignals(tab);
        tab->part->openUrl(initialUrl.isEmpty() ? QUrl::fromLocalFile(QDir::homePath()) : initialUrl);
    }

    m_tabs->setCurrentIndex(index);
    return index;
}

void ShellWindow::closeTab(int index)
{
    Tab *tab = tabAt(index);
    if (!tab) {
        return;
    }
    if (m_tabs->count() == 1) {
        close();
        return;
    }
    if (m_tabs->currentIndex() == index) {
        m_tabs->setCurrentIndex(index == 0 ? 1 : index - 1);
    }
    m_tabs->removeTab(index);
    m_stack->removeWidget(tab->host);
    m_tabsData.removeAll(tab);
    if (tab->part) {
        tab->part->deleteLater();
    }
    tab->host->deleteLater();
    delete tab;
    updateNavButtons();
}

void ShellWindow::switchToTab(int index)
{
    Tab *tab = tabAt(index);
    if (!tab) {
        return;
    }
    m_stack->setCurrentWidget(tab->host);
    syncUrl(tab);
    rebindToCurrentPart();
}

void ShellWindow::connectPartSignals(ShellWindow::Tab *tab)
{
    KParts::ReadOnlyPart *part = tab->part;
    if (!part) {
        return;
    }

    connect(part, &KParts::ReadOnlyPart::completed, this, [this, part] {
        if (Tab *tab = tabOfPart(part)) {
            recordUrl(tab, part->url());
            syncUrl(tab);
        }
    });
    connect(part, &KParts::ReadOnlyPart::setWindowCaption, this, [this, part](const QString &caption) {
        if (Tab *tab = tabOfPart(part)) {
            updateTabText(tab, caption);
        }
    });
    connect(part, &KParts::ReadOnlyPart::setStatusBarText, this, [this, part](const QString &text) {
        if (Tab *tab = tabOfPart(part); tab && tab == currentTab()) {
            m_statusLabel->setText(text);
        }
    });

    auto *navigation = qobject_cast<KParts::NavigationExtension *>(part->navigationExtension());
    if (!navigation) {
        return;
    }

    connect(navigation, &KParts::NavigationExtension::setLocationBarUrl, this, [this, part](const QString &prettyUrl) {
        if (Tab *tab = tabOfPart(part)) {
            updateTabText(tab, prettyUrl);
            const QUrl parsed = QUrl::fromUserInput(prettyUrl);
            if (tab == currentTab() && !parsed.isEmpty()) {
                m_breadcrumb->setUrl(parsed);
                m_sidebar->highlight(parsed);
            }
        }
    });
    connect(navigation, &KParts::NavigationExtension::createNewWindow, this, [this](const QUrl &url) {
        addTab(url);
    });
    connect(navigation, &KParts::NavigationExtension::openUrlRequest, this,
            [this](const QUrl &url, const KParts::OpenUrlArguments &) {
                static const QSet<QString> directorySchemes = {
                    QStringLiteral("trash"), QStringLiteral("remote"), QStringLiteral("programs"),
                    QStringLiteral("settings"), QStringLiteral("network"), QStringLiteral("applications"),
                    QStringLiteral("timeline"), QStringLiteral("recentdocuments"), QStringLiteral("recentlyused"),
                };
                const bool localDir = url.isLocalFile() && QFileInfo(url.toLocalFile()).isDir();
                const bool looksLikeDir = url.path().isEmpty() || url.path().endsWith(QLatin1Char('/'))
                    || directorySchemes.contains(url.scheme());
                if (localDir || looksLikeDir) {
                    navigate(url);
                } else {
                    openItemUrl(url);
                }
            });
    connect(navigation, &KParts::NavigationExtension::enableAction, this,
            [this](const char *name, bool enabled) {
                const QByteArray actionName(name);
                if (actionName == "cut") {
                    m_cutAction->setEnabled(enabled);
                } else if (actionName == "copy") {
                    m_copyAction->setEnabled(enabled);
                } else if (actionName == "paste") {
                    m_pasteAction->setEnabled(enabled);
                }
            });
    connect(navigation, &KParts::NavigationExtension::setActionText, this,
            [this](const char *name, const QString &text) {
                if (QByteArray(name) == "paste") {
                    m_pasteAction->setText(text);
                }
            });

    // The view asks for history navigation (mouse back/forward buttons, Alt+Up...)
    connect(part->widget(), SIGNAL(goBackRequested()), this, SLOT(goBack()));
    connect(part->widget(), SIGNAL(goForwardRequested()), this, SLOT(goForward()));
    connect(part->widget(), SIGNAL(goUpRequested()), this, SLOT(goUp()));
    connect(navigation,
            qOverload<const QPoint &,
                      const KFileItemList &,
                      const KParts::OpenUrlArguments &,
                      KParts::NavigationExtension::PopupFlags,
                      const KParts::NavigationExtension::ActionGroupMap &>(&KParts::NavigationExtension::popupMenu),
            this,
            [this, part](const QPoint &globalPos, const KFileItemList &items, const KParts::OpenUrlArguments &,
                         KParts::NavigationExtension::PopupFlags flags,
                         const KParts::NavigationExtension::ActionGroupMap &actionGroups) {
                Q_UNUSED(flags)
                const QUrl current = part->url();
                const bool background = items.isEmpty()
                    || (items.size() == 1 && items.first().url().matches(current, QUrl::StripTrailingSlash));

                QMenu menu(this);
                if (!background && items.size() == 1) {
                    const KFileItem item = items.first();
                    if (item.isDir()) {
                        QAction *openAction = menu.addAction(
                            Theme::icon(QStringLiteral("document-open-folder")), QStringLiteral("Open"));
                        connect(openAction, &QAction::triggered, this, [this, url = item.url()] { navigate(url); });
                        QAction *tabAction = menu.addAction(
                            Theme::icon(QStringLiteral("tab-new"), QStringLiteral("list-add")),
                            QStringLiteral("Open in New Tab"));
                        connect(tabAction, &QAction::triggered, this, [this, url = item.url()] { addTab(url); });
                    } else {
                        QAction *openAction = menu.addAction(
                            Theme::icon(QStringLiteral("document-open")), QStringLiteral("Open"));
                        connect(openAction, &QAction::triggered, this, [this, url = item.url()] { openItemUrl(url); });
                    }
                    menu.addSeparator();
                }
                for (QAction *action : actionGroups.value(QStringLiteral("editactions"))) {
                    menu.addAction(action);
                }
                if (background) {
                    menu.addSeparator();
                    QAction *newMenu = part->actionCollection()->action(QStringLiteral("new_menu"));
                    if (!newMenu) {
                        newMenu = part->actionCollection()->action(QStringLiteral("create_dir"));
                    }
                    if (newMenu) {
                        menu.addAction(newMenu);
                    }
                    if (QAction *paste = part->actionCollection()->action(QStringLiteral("paste"))) {
                        menu.addAction(paste);
                    }
                    QAction *newTab = menu.addAction(
                        Theme::icon(QStringLiteral("tab-new"), QStringLiteral("list-add")),
                        QStringLiteral("Open in New Tab"));
                    connect(newTab, &QAction::triggered, this, [this, current] { addTab(current); });
                }
                if (QAction *props = part->actionCollection()->action(QStringLiteral("properties"))) {
                    menu.addSeparator();
                    menu.addAction(props);
                }
                menu.exec(globalPos);
            });
}

void ShellWindow::rebindToCurrentPart()
{
    for (Tab *tab : m_tabsData) {
        if (tab->part) {
            const QList<QAction *> actions = tab->part->actionCollection()->actions();
            for (QAction *action : actions) {
                removeAction(action);
            }
        }
    }
    if (KParts::ReadOnlyPart *part = currentPart()) {
        addActions(part->actionCollection()->actions());
    }

    if (QMenu *oldMenu = m_newButton->menu()) {
        oldMenu->deleteLater();
    }
    auto *newMenu = new QMenu(m_newButton);
    bool hasNewEntries = false;
    QAction *createDir = partAction("create_dir");
    QAction *createSubdir = partAction("create_subdir");
    QAction *createFile = partAction("create_file");
    if (createDir) {
        newMenu->addAction(createDir);
        hasNewEntries = true;
    }
    if (createSubdir) {
        newMenu->addAction(createSubdir);
        hasNewEntries = true;
    }
    if (createFile) {
        if (createFile->menu()) {
            QMenu *fileMenu = newMenu->addMenu(createFile->text());
            fileMenu->setIcon(createFile->icon());
            const QList<QAction *> templates = createFile->menu()->actions();
            for (QAction *action : templates) {
                fileMenu->addAction(action);
            }
        } else {
            newMenu->addAction(createFile);
        }
        hasNewEntries = true;
    }
    m_newButton->setMenu(newMenu);
    m_newButton->setPopupMode(QToolButton::InstantPopup);
    m_newButton->setVisible(hasNewEntries);
    m_newButton->setIcon(Theme::icon(QStringLiteral("list-add"), QStringLiteral("folder-new")));
    m_newButton->setText(QStringLiteral("New"));
    m_newButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_cutButton->setDefaultAction(m_cutAction);
    m_copyButton->setDefaultAction(m_copyAction);
    m_pasteButton->setDefaultAction(m_pasteAction);
    m_renameButton->setDefaultAction(partAction("renamefile"));
    m_deleteButton->setDefaultAction(partAction("movetotrash"));
    if (!m_deleteButton->defaultAction()) {
        m_deleteButton->setDefaultAction(partAction("deletefile"));
    }

    m_cutButton->setVisible(true);
    m_copyButton->setVisible(true);
    m_pasteButton->setVisible(true);
    m_renameButton->setVisible(m_renameButton->defaultAction() != nullptr);
    m_deleteButton->setVisible(m_deleteButton->defaultAction() != nullptr);

    rebuildSortMenu();
    rebuildViewMenu();
    rebuildMoreMenu();
    updateNavButtons();
}

void ShellWindow::navigate(const QUrl &url)
{
    Tab *tab = currentTab();
    if (!tab || !tab->part || url.isEmpty()) {
        return;
    }
    tab->skipRecord = false;
    tab->part->openUrl(url);
    syncUrl(tab);
}

void ShellWindow::goBack()
{
    Tab *tab = currentTab();
    if (!tab || !tab->part || tab->historyPos <= 0) {
        return;
    }
    --tab->historyPos;
    tab->skipRecord = true;
    tab->part->openUrl(tab->history.at(tab->historyPos));
    syncUrl(tab);
}

void ShellWindow::goForward()
{
    Tab *tab = currentTab();
    if (!tab || !tab->part || tab->historyPos < 0 || tab->historyPos >= tab->history.size() - 1) {
        return;
    }
    ++tab->historyPos;
    tab->skipRecord = true;
    tab->part->openUrl(tab->history.at(tab->historyPos));
    syncUrl(tab);
}

void ShellWindow::goUp()
{
    Tab *tab = currentTab();
    if (!tab || !tab->part) {
        return;
    }
    const QUrl current = tab->part->url();
    if (current.isLocalFile()) {
        QDir dir(current.toLocalFile());
        if (dir.cdUp()) {
            navigate(QUrl::fromLocalFile(dir.absolutePath()));
        }
        return;
    }
    QString path = current.path();
    if (!path.isEmpty() && path != QLatin1String("/")) {
        QStringList segments = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        segments.removeLast();
        QUrl up = current;
        up.setPath(QLatin1Char('/') + segments.join(QLatin1Char('/')));
        navigate(up);
    } else {
        navigate(QUrl::fromLocalFile(QDir::homePath()));
    }
}

void ShellWindow::syncUrl(ShellWindow::Tab *tab)
{
    if (!tab || !tab->part) {
        return;
    }
    const QUrl url = tab->part->url();
    const int index = m_tabsData.indexOf(tab);
    if (index >= 0) {
        QString iconName = QStringLiteral("folder");
        if (url == QUrl::fromLocalFile(QDir::homePath())) {
            iconName = QStringLiteral("user-home");
        } else if (url.scheme() == QLatin1String("trash")) {
            iconName = QStringLiteral("user-trash");
        } else if (url.scheme() == QLatin1String("remote")) {
            iconName = QStringLiteral("folder-remote");
        }
        m_tabs->setTabIcon(index, Theme::icon(iconName));
    }
    if (tab == currentTab()) {
        m_breadcrumb->setUrl(url);
        m_sidebar->highlight(url);
        updateNavButtons();
    }
}

void ShellWindow::recordUrl(ShellWindow::Tab *tab, const QUrl &url)
{
    if (!tab) {
        return;
    }
    if (tab->skipRecord) {
        tab->skipRecord = false;
        updateNavButtons();
        return;
    }
    QUrl clean = url;
    clean = clean.adjusted(QUrl::StripTrailingSlash);
    while (tab->history.size() > tab->historyPos + 1) {
        tab->history.removeLast();
    }
    if (tab->history.isEmpty() || tab->history.last() != clean) {
        tab->history.append(clean);
        tab->historyPos = tab->history.size() - 1;
    }
    updateNavButtons();
}

void ShellWindow::updateNavButtons()
{
    Tab *tab = currentTab();
    m_backAction->setEnabled(tab && tab->historyPos > 0);
    m_forwardAction->setEnabled(tab && tab->historyPos >= 0 && tab->historyPos < tab->history.size() - 1);
}

void ShellWindow::updateTabText(ShellWindow::Tab *tab, const QString &caption)
{
    const int index = m_tabsData.indexOf(tab);
    if (index < 0) {
        return;
    }
    QString shortName = caption;
    if (shortName.endsWith(QLatin1Char('/'))) {
        shortName.chop(1);
    }
    if (shortName.endsWith(QLatin1Char(':'))) {
        shortName.chop(1);
    }
    const int slash = shortName.lastIndexOf(QLatin1Char('/'));
    if (slash >= 0 && slash + 1 < shortName.size()) {
        shortName = shortName.mid(slash + 1);
    }
    if (shortName.isEmpty()) {
        shortName = caption.isEmpty() ? QStringLiteral("Home") : caption;
    } else {
        shortName[0] = shortName.at(0).toUpper();
    }
    m_tabs->setTabText(index, shortName);
    m_tabs->setTabToolTip(index, caption);
    if (tab == currentTab()) {
        setWindowTitle(QStringLiteral("%1 — Dolphin Shell").arg(shortName));
    }
}

QAction *ShellWindow::partAction(const char *name) const
{
    KParts::ReadOnlyPart *part = currentPart();
    return part ? part->actionCollection()->action(QString::fromLatin1(name)) : nullptr;
}

void ShellWindow::onSearchSubmitted()
{
    Tab *tab = currentTab();
    if (!tab || !tab->part) {
        return;
    }
    const QString text = m_searchEdit->text().trimmed();
    if (!text.isEmpty()) {
        QAction *find = tab->part->actionCollection()->action(QStringLiteral("edit_find"));
        if (find && find->isEnabled()) {
            find->trigger();
        }
    }
}

void ShellWindow::rebuildSortMenu()
{
    if (QMenu *oldMenu = m_sortButton->menu()) {
        oldMenu->deleteLater();
    }
    auto *menu = new QMenu(m_sortButton);
    bool hasEntries = false;

    if (KParts::ReadOnlyPart *part = currentPart()) {
        QList<QAction *> sortActions;
        const QList<QAction *> actions = part->actionCollection()->actions();
        for (QAction *action : actions) {
            const QString name = action->objectName();
            if (name.startsWith(QLatin1String("sort_by_")) || name == QLatin1String("sort_descending")) {
                sortActions.append(action);
            }
        }
        if (!sortActions.isEmpty()) {
            for (QAction *action : std::as_const(sortActions)) {
                menu->addAction(action);
            }
            hasEntries = true;
        } else if (QAction *sortAction = part->actionCollection()->action(QStringLiteral("sort"))) {
            if (sortAction->menu() && !sortAction->menu()->isEmpty()) {
                menu->addMenu(sortAction->menu());
                hasEntries = true;
            }
        }
    }

    m_sortButton->setMenu(menu);
    m_sortButton->setVisible(hasEntries);
    m_sortButton->setText(QStringLiteral("Sort"));
    m_sortButton->setIcon(Theme::icon(QStringLiteral("view-sort-ascending")));
    m_sortButton->setToolTip(QStringLiteral("Sort by"));
    m_sortButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_sortButton->setPopupMode(QToolButton::InstantPopup);
}

void ShellWindow::rebuildViewMenu()
{
    if (QMenu *oldMenu = m_viewButton->menu()) {
        oldMenu->deleteLater();
    }
    auto *menu = new QMenu(m_viewButton);

    if (KParts::ReadOnlyPart *part = currentPart()) {
        QAction *icons = part->actionCollection()->action(QStringLiteral("icons"));
        QAction *compact = part->actionCollection()->action(QStringLiteral("compact"));
        QAction *details = part->actionCollection()->action(QStringLiteral("details"));
        if (icons) {
            menu->addAction(icons);
        }
        if (compact) {
            menu->addAction(compact);
        }
        if (details) {
            menu->addAction(details);
        }
        if (icons || compact || details) {
            menu->addSeparator();
        }
        if (QAction *hidden = part->actionCollection()->action(QStringLiteral("show_hidden_files"))) {
            menu->addAction(hidden);
        }
        if (QAction *preview = part->actionCollection()->action(QStringLiteral("show_preview"))) {
            menu->addAction(preview);
        }
        menu->addSeparator();
    }

    menu->addAction(m_sidebarAction);
    menu->addAction(m_themeAction);
    menu->addSeparator();
    QAction *openIn = menu->addAction(
        Theme::icon(QStringLiteral("system-file-manager"), QStringLiteral("folder")),
        QStringLiteral("Open in Dolphin"));
    connect(openIn, &QAction::triggered, this, &ShellWindow::openInDolphin);
    QAction *about = menu->addAction(Theme::icon(QStringLiteral("help-about")), QStringLiteral("About Dolphin Shell"));
    connect(about, &QAction::triggered, this, &ShellWindow::showAbout);

    m_viewButton->setMenu(menu);
    m_viewButton->setText(QStringLiteral("View"));
    m_viewButton->setIcon(Theme::icon(QStringLiteral("view-choose"), QStringLiteral("preferences-desktop-view")));
    m_viewButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_viewButton->setPopupMode(QToolButton::InstantPopup);
}

void ShellWindow::rebuildMoreMenu()
{
    if (QMenu *oldMenu = m_moreButton->menu()) {
        oldMenu->deleteLater();
    }
    auto *menu = new QMenu(m_moreButton);

    if (KParts::ReadOnlyPart *part = currentPart()) {
        if (QAction *find = part->actionCollection()->action(QStringLiteral("edit_find"))) {
            menu->addAction(find);
        }
        if (QAction *terminal = part->actionCollection()->action(QStringLiteral("open_terminal"))) {
            menu->addAction(terminal);
        }
        menu->addSeparator();
        if (QAction *selectAll = part->actionCollection()->action(QStringLiteral("edit_select_all"))) {
            menu->addAction(selectAll);
        }
        if (QAction *invert = part->actionCollection()->action(QStringLiteral("invert_selection"))) {
            menu->addAction(invert);
        }
        if (QAction *props = part->actionCollection()->action(QStringLiteral("properties"))) {
            menu->addAction(props);
        }
        if (QAction *duplicate = part->actionCollection()->action(QStringLiteral("duplicate"))) {
            menu->addAction(duplicate);
        }
        menu->addSeparator();
    }

    QAction *newTab = menu->addAction(
        Theme::icon(QStringLiteral("tab-new"), QStringLiteral("list-add")), QStringLiteral("New Tab"));
    newTab->setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));
    connect(newTab, &QAction::triggered, this, [this] {
        Tab *tab = currentTab();
        addTab(tab && tab->part ? tab->part->url() : QUrl());
    });
    QAction *openIn = menu->addAction(
        Theme::icon(QStringLiteral("system-file-manager"), QStringLiteral("folder")),
        QStringLiteral("Open in Dolphin"));
    connect(openIn, &QAction::triggered, this, &ShellWindow::openInDolphin);
    menu->addSeparator();
    QAction *quit = menu->addAction(QStringLiteral("Quit"));
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QWidget::close);

    m_moreButton->setMenu(menu);
    m_moreButton->setIcon(Theme::icon(QStringLiteral("overflow-menu")));
    m_moreButton->setText(QString());
    m_moreButton->setToolTip(QStringLiteral("More options"));
    m_moreButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_moreButton->setPopupMode(QToolButton::InstantPopup);
}

void ShellWindow::applyTheme(Theme::Mode mode)
{
    m_mode = mode;
    Theme::saveMode(mode);
    qApp->setProperty("dolphinShellDark", mode == Theme::Mode::Dark);
    qApp->setPalette(Theme::palette(mode));
    qApp->setStyleSheet(Theme::styleSheet(mode));

    const bool dark = (mode == Theme::Mode::Dark);
    if (m_themeAction) {
        m_themeAction->setChecked(dark);
        m_themeAction->setIcon(Theme::icon(dark ? QStringLiteral("weather-clear-night") : QStringLiteral("weather-clear")));
    }
    if (m_themeButton) {
        m_themeButton->setIcon(Theme::icon(dark ? QStringLiteral("weather-clear-night") : QStringLiteral("weather-clear")));
        m_themeButton->setToolTip(dark ? QStringLiteral("Switch to light mode") : QStringLiteral("Switch to dark mode"));
    }
    if (m_sidebar) {
        m_sidebar->refresh();
    }
    if (m_controls) {
        m_controls->update();
    }
}

void ShellWindow::toggleTheme()
{
    applyTheme(m_mode == Theme::Mode::Dark ? Theme::Mode::Light : Theme::Mode::Dark);
}

void ShellWindow::openInDolphin()
{
    KParts::ReadOnlyPart *part = currentPart();
    if (!part) {
        return;
    }
    if (!QProcess::startDetached(QStringLiteral("dolphin"), {part->url().toString()})) {
        m_statusLabel->setText(QStringLiteral("Could not start Dolphin."));
    }
}

void ShellWindow::showAbout()
{
    QMessageBox::about(this, QStringLiteral("About Dolphin Shell"),
                       QStringLiteral("<h3>Dolphin Shell 0.1.0</h3>"
                                      "<p>A Windows 11 Explorer style shell for Dolphin.</p>"
                                      "<p>Made by AzenDev.</p>"));
}

void ShellWindow::openItemUrl(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

void ShellWindow::openInitialUrl(const QUrl &url)
{
    if (!url.isEmpty() && currentPart()) {
        currentPart()->openUrl(url);
        syncUrl(currentTab());
    }
}

void ShellWindow::dumpActionsAndQuit()
{
    QTimer::singleShot(600, this, [this] {
        fprintf(stderr, "=== DolphinPart actions ===\n");
        for (Tab *tab : m_tabsData) {
            if (!tab->part) {
                continue;
            }
            const QList<QAction *> actions = tab->part->actionCollection()->actions();
            for (const QAction *action : actions) {
                fprintf(stderr, "%s | %s | %s%s\n",
                        qPrintable(action->objectName()),
                        qPrintable(action->text()),
                        action->menu() ? "HASMENU " : "",
                        action->isCheckable() ? "CHECKABLE" : "");
            }
            break;
        }
        QTimer::singleShot(100, qApp, &QCoreApplication::quit);
    });
}

bool ShellWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_tabs) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton && m_tabs->tabAt(mouseEvent->pos()) < 0) {
                if (event->type() == QEvent::MouseButtonDblClick) {
                    setWindowState(isMaximized() ? Qt::WindowNoState : Qt::WindowMaximized);
                    return true;
                }
                if (windowHandle()) {
                    windowHandle()->startSystemMove();
                    return true;
                }
            }
        }
    } else if (watched == m_titleBar) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (event->type() == QEvent::MouseButtonDblClick) {
                    setWindowState(isMaximized() ? Qt::WindowNoState : Qt::WindowMaximized);
                    return true;
                }
                if (windowHandle()) {
                    windowHandle()->startSystemMove();
                    return true;
                }
            }
        }
    } else if (event->type() == QEvent::MouseButtonPress && !isMaximized()) {
        if (watched == m_root || watched == m_commandBar || watched == m_splitter || watched == m_statusBar) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton && windowHandle()) {
                auto *widget = qobject_cast<QWidget *>(watched);
                if (widget) {
                    const QPoint pos = widget->mapTo(m_root, mouseEvent->pos());
                    const int margin = 8;
                    Qt::Edges edges;
                    if (pos.x() <= margin) {
                        edges |= Qt::LeftEdge;
                    }
                    if (pos.x() >= m_root->width() - margin) {
                        edges |= Qt::RightEdge;
                    }
                    if (pos.y() <= margin) {
                        edges |= Qt::TopEdge;
                    }
                    if (pos.y() >= m_root->height() - margin) {
                        edges |= Qt::BottomEdge;
                    }
                    if (edges != Qt::Edges()) {
                        windowHandle()->startSystemResize(edges);
                        return true;
                    }
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void ShellWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        m_root->setProperty("maximized", isMaximized());
        m_root->style()->unpolish(m_root);
        m_root->style()->polish(m_root);
        m_controls->setMaximized(isMaximized());
    }
}

void ShellWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
    settings.setValue(QStringLiteral("sidebarVisible"), m_sidebar->isVisible());
    QMainWindow::closeEvent(event);
}
