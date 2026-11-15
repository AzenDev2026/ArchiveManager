#pragma once

#include <QWidget>

class WindowControls : public QWidget
{
    Q_OBJECT
public:
    enum Button { Minimize = 0, Maximize = 1, Close = 2 };

    explicit WindowControls(QWidget *parent = nullptr);

    void setMaximized(bool maximized);
    QSize sizeHint() const override;

signals:
    void clicked(int button);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void enterEvent(QEnterEvent *event) override;

private:
    QRect buttonRect(int index) const;
    int buttonAt(const QPoint &pos) const;

    int m_hovered = -1;
    bool m_pressed = false;
    int m_pressedButton = -1;
    bool m_maximized = false;
};
