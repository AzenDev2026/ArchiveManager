#pragma once

#include <QUrl>
#include <QWidget>

class QHBoxLayout;
class QLineEdit;
class QMouseEvent;
class QResizeEvent;

class Breadcrumb : public QWidget
{
    Q_OBJECT
public:
    explicit Breadcrumb(QWidget *parent = nullptr);

    void setUrl(const QUrl &url);
    QUrl url() const { return m_url; }
    void enterEditMode();

signals:
    void navigateRequested(const QUrl &url);
    void textSubmitted(const QString &text);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void rebuild();
    void leaveEditMode(bool accept);

    QUrl m_url;
    QHBoxLayout *m_layout = nullptr;
    QLineEdit *m_edit = nullptr;
    bool m_compact = false;
};
