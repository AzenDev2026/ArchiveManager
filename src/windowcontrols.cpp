#include "windowcontrols.h"

#include <QApplication>
#include <QCursor>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>

WindowControls::WindowControls(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(32);
    setFixedWidth(138);
    setMouseTracking(true);
}

QSize WindowControls::sizeHint() const
{
    return QSize(138, 32);
}

void WindowControls::setMaximized(bool maximized)
{
    if (m_maximized == maximized) {
        return;
    }
    m_maximized = maximized;
    update();
}

QRect WindowControls::buttonRect(int index) const
{
    return QRect(index * 46, 0, 46, height());
}

int WindowControls::buttonAt(const QPoint &pos) const
{
    for (int i = 0; i < 3; ++i) {
        if (buttonRect(i).contains(pos)) {
            return i;
        }
    }
    return -1;
}

void WindowControls::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const bool dark = qApp->property("dolphinShellDark").toBool();
    const QColor textColor = palette().color(QPalette::WindowText);

    for (int i = 0; i < 3; ++i) {
        const QRect r = buttonRect(i);
        const bool hovered = (m_hovered == i);
        const bool pressed = m_pressed && m_pressedButton == i && hovered;
        if (hovered) {
            QColor bg;
            if (i == Close) {
                bg = pressed ? QColor(0xb0, 0x26, 0x1b) : QColor(0xc4, 0x2b, 0x1c);
            } else {
                bg = dark ? (pressed ? QColor(255, 255, 255, 32) : QColor(255, 255, 255, 20))
                          : (pressed ? QColor(0, 0, 0, 26) : QColor(0, 0, 0, 16));
            }
            p.fillRect(r, bg);
        }
        QColor fg = textColor;
        if (i == Close && hovered) {
            fg = Qt::white;
        }
        p.setPen(QPen(fg, 1.3));
        p.setBrush(Qt::NoBrush);
        const QPointF c(r.center());
        switch (i) {
        case Minimize:
            p.drawLine(QPointF(c.x() - 5, c.y() + 0.5), QPointF(c.x() + 5, c.y() + 0.5));
            break;
        case Maximize:
            if (!m_maximized) {
                p.drawRoundedRect(QRectF(c.x() - 5, c.y() - 5, 10, 10), 1.5, 1.5);
            } else {
                p.drawRoundedRect(QRectF(c.x() - 6, c.y() - 3, 8, 8), 1.5, 1.5);
                p.drawRoundedRect(QRectF(c.x() - 2, c.y() - 7, 8, 8), 1.5, 1.5);
            }
            break;
        case Close:
            p.drawLine(QPointF(c.x() - 4.5, c.y() - 4.5), QPointF(c.x() + 4.5, c.y() + 4.5));
            p.drawLine(QPointF(c.x() - 4.5, c.y() + 4.5), QPointF(c.x() + 4.5, c.y() - 4.5));
            break;
        }
    }
}

void WindowControls::mouseMoveEvent(QMouseEvent *event)
{
    const int b = buttonAt(event->pos());
    if (b != m_hovered) {
        m_hovered = b;
        update();
    }
}

void WindowControls::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        m_pressedButton = buttonAt(event->pos());
        update();
    }
}

void WindowControls::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_pressed) {
        const int released = buttonAt(event->pos());
        if (released == m_pressedButton && released != -1) {
            emit clicked(released);
        }
        m_pressed = false;
        m_pressedButton = -1;
        update();
    }
}

void WindowControls::leaveEvent(QEvent *)
{
    if (m_hovered != -1) {
        m_hovered = -1;
        update();
    }
}

void WindowControls::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    m_hovered = buttonAt(mapFromGlobal(QCursor::pos()));
    update();
}
