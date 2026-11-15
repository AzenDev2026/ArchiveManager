#include "sidebar.h"

#include <QApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPalette>
#include <QSet>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QToolButton>
#include <QVBoxLayout>

#include "theme.h"

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(0);

    auto *headerRow = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(8, 2, 2, 2);
    headerLayout->setSpacing(2);

    auto *title = new QLabel(QStringLiteral("Navigation"), headerRow);
    headerLayout->addWidget(title, 1);

    auto *collapseButton = new QToolButton(headerRow);
    collapseButton->setObjectName(QStringLiteral("sidebarToggle"));
    collapseButton->setText(QStringLiteral("«"));
    collapseButton->setToolTip(QStringLiteral("Collapse the navigation pane"));
    headerLayout->addWidget(collapseButton);
    connect(collapseButton, &QToolButton::clicked, this, &Sidebar::collapseRequested);

    layout->addWidget(headerRow);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("sidebar"));
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_list, 1);

    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        const QUrl url = item->data(Qt::UserRole).toUrl();
        if (!url.isEmpty()) {
            emit navigateRequested(url);
        }
    });

    populate();
}

void Sidebar::refresh()
{
    populate();
}

void Sidebar::populate()
{
    m_list->clear();

    addPlace(QStringLiteral("Home"), QUrl::fromLocalFile(QDir::homePath()), QStringLiteral("user-home"));
    addHeader(QStringLiteral("Quick access"));

    const QList<QPair<QString, QString>> standardPlaces = {
        {QStringLiteral("Desktop"), QStringLiteral("user-desktop")},
        {QStringLiteral("Documents"), QStringLiteral("folder-documents")},
        {QStringLiteral("Downloads"), QStringLiteral("folder-download")},
        {QStringLiteral("Music"), QStringLiteral("folder-music")},
        {QStringLiteral("Pictures"), QStringLiteral("folder-pictures")},
        {QStringLiteral("Videos"), QStringLiteral("folder-videos")},
    };
    const QList<QStandardPaths::StandardLocation> locations = {
        QStandardPaths::DesktopLocation,
        QStandardPaths::DocumentsLocation,
        QStandardPaths::DownloadLocation,
        QStandardPaths::MusicLocation,
        QStandardPaths::PicturesLocation,
        QStandardPaths::MoviesLocation,
    };
    for (int i = 0; i < standardPlaces.size() && i < locations.size(); ++i) {
        const QString path = QStandardPaths::writableLocation(locations.at(i));
        if (!path.isEmpty() && QDir(path).exists()) {
            addPlace(standardPlaces.at(i).first, QUrl::fromLocalFile(path), standardPlaces.at(i).second);
        }
    }

    addHeader(QStringLiteral("This PC"));
    addPlace(QStringLiteral("File System"),
             QUrl::fromLocalFile(QStringLiteral("/")),
             QStringLiteral("computer"));

    static const QSet<QString> skipFileSystems = {
        QStringLiteral("tmpfs"),       QStringLiteral("devtmpfs"), QStringLiteral("proc"),
        QStringLiteral("sysfs"),       QStringLiteral("cgroup"),  QStringLiteral("cgroup2"),
        QStringLiteral("overlay"),     QStringLiteral("squashfs"), QStringLiteral("ramfs"),
        QStringLiteral("bpf"),         QStringLiteral("efivarfs"), QStringLiteral("tracefs"),
        QStringLiteral("securityfs"),  QStringLiteral("debugfs"), QStringLiteral("configfs"),
        QStringLiteral("fusectl"),     QStringLiteral("pstore"),  QStringLiteral("mqueue"),
        QStringLiteral("hugetlbfs"),   QStringLiteral("binfmt_misc"), QStringLiteral("autofs"),
        QStringLiteral("nsfs"),        QStringLiteral("fuse.portal"),
    };
    QSet<QString> seenDevices;
    const QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &volume : volumes) {
        const QString fileSystem = volume.fileSystemType();
        const QString rootPath = volume.rootPath();
        const QString device = volume.device();
        if (skipFileSystems.contains(fileSystem)) {
            continue;
        }
        if (rootPath == QLatin1String("/") || rootPath.startsWith(QLatin1String("/run/user"))) {
            continue;
        }
        const QString key = device.isEmpty() ? rootPath : device;
        if (!device.isEmpty() && seenDevices.contains(key)) {
            continue;
        }
        seenDevices.insert(key);
        QString label = volume.displayName();
        if (label.isEmpty()) {
            label = device;
        }
        if (label.isEmpty()) {
            label = rootPath;
        }
        addPlace(label, QUrl::fromLocalFile(rootPath), QStringLiteral("drive-harddisk"));
    }

    addHeader(QStringLiteral("Locations"));
    addPlace(QStringLiteral("Network"), QUrl(QStringLiteral("remote:/")), QStringLiteral("folder-remote"));
    addPlace(QStringLiteral("Trash"), QUrl(QStringLiteral("trash:/")), QStringLiteral("user-trash"));
}

void Sidebar::addHeader(const QString &text)
{
    auto *item = new QListWidgetItem(text, m_list);
    item->setFlags(Qt::NoItemFlags);
    QFont f = font();
    f.setBold(true);
    f.setPixelSize(11);
    item->setFont(f);
    item->setForeground(qApp->palette().color(QPalette::Disabled, QPalette::Text));
    item->setSizeHint(QSize(0, 26));
}

void Sidebar::addPlace(const QString &text, const QUrl &url, const QString &iconName)
{
    auto *item = new QListWidgetItem(Theme::icon(iconName), text, m_list);
    item->setData(Qt::UserRole, url);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

void Sidebar::highlight(const QUrl &url)
{
    const QSignalBlocker blocker(m_list);
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        const QUrl itemUrl = item->data(Qt::UserRole).toUrl();
        const bool match = !itemUrl.isEmpty()
            && (itemUrl.matches(url, QUrl::StripTrailingSlash) || url.matches(itemUrl, QUrl::StripTrailingSlash));
        if (match) {
            m_list->setCurrentItem(item);
            m_list->scrollToItem(item);
            break;
        }
    }
}
