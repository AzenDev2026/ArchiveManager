#pragma once

#include <QUrl>
#include <QWidget>

class QListWidget;
class QListWidgetItem;

class Sidebar : public QWidget
{
    Q_OBJECT
public:
    explicit Sidebar(QWidget *parent = nullptr);

    void highlight(const QUrl &url);
    void refresh();

signals:
    void navigateRequested(const QUrl &url);
    void collapseRequested();

private:
    void populate();
    void addHeader(const QString &text);
    void addPlace(const QString &text, const QUrl &url, const QString &iconName);

    QListWidget *m_list = nullptr;
};
