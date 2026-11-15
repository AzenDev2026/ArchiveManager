#include "theme.h"

#include <QSettings>

namespace Theme {

Mode loadMode()
{
    const QString value = QSettings().value(QStringLiteral("theme"), QStringLiteral("dark")).toString();
    return value == QLatin1String("light") ? Mode::Light : Mode::Dark;
}

void saveMode(Mode mode)
{
    QSettings().setValue(QStringLiteral("theme"), mode == Mode::Dark ? QStringLiteral("dark") : QStringLiteral("light"));
}

QPalette palette(Mode mode)
{
    QPalette p;
    if (mode == Mode::Dark) {
        p.setColor(QPalette::Window, QColor(0x20, 0x20, 0x20));
        p.setColor(QPalette::WindowText, QColor(0xf0, 0xf0, 0xf0));
        p.setColor(QPalette::Base, QColor(0x2b, 0x2b, 0x2b));
        p.setColor(QPalette::AlternateBase, QColor(0x26, 0x26, 0x26));
        p.setColor(QPalette::ToolTipBase, QColor(0x2c, 0x2c, 0x2c));
        p.setColor(QPalette::ToolTipText, QColor(0xf0, 0xf0, 0xf0));
        p.setColor(QPalette::Text, QColor(0xf0, 0xf0, 0xf0));
        p.setColor(QPalette::Button, QColor(0x2b, 0x2b, 0x2b));
        p.setColor(QPalette::ButtonText, QColor(0xf0, 0xf0, 0xf0));
        p.setColor(QPalette::BrightText, QColor(0xff, 0x60, 0x60));
        p.setColor(QPalette::Link, QColor(0x4c, 0xc2, 0xff));
        p.setColor(QPalette::Highlight, QColor(0x00, 0x63, 0xb0));
        p.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        p.setColor(QPalette::PlaceholderText, QColor(0x9a, 0x9a, 0x9a));
        p.setColor(QPalette::Disabled, QPalette::Text, QColor(0x8a, 0x8a, 0x8a));
        p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x8a, 0x8a, 0x8a));
        p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x8a, 0x8a, 0x8a));
    } else {
        p.setColor(QPalette::Window, QColor(0xf3, 0xf3, 0xf3));
        p.setColor(QPalette::WindowText, QColor(0x1a, 0x1a, 0x1a));
        p.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
        p.setColor(QPalette::AlternateBase, QColor(0xf9, 0xf9, 0xf9));
        p.setColor(QPalette::ToolTipBase, QColor(0xff, 0xff, 0xff));
        p.setColor(QPalette::ToolTipText, QColor(0x1a, 0x1a, 0x1a));
        p.setColor(QPalette::Text, QColor(0x1a, 0x1a, 0x1a));
        p.setColor(QPalette::Button, QColor(0xff, 0xff, 0xff));
        p.setColor(QPalette::ButtonText, QColor(0x1a, 0x1a, 0x1a));
        p.setColor(QPalette::BrightText, QColor(0xc4, 0x2b, 0x1c));
        p.setColor(QPalette::Link, QColor(0x00, 0x5f, 0xb8));
        p.setColor(QPalette::Highlight, QColor(0x00, 0x63, 0xb0));
        p.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        p.setColor(QPalette::PlaceholderText, QColor(0x78, 0x78, 0x78));
        p.setColor(QPalette::Disabled, QPalette::Text, QColor(0xa0, 0xa0, 0xa0));
        p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0xa0, 0xa0, 0xa0));
        p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0xa0, 0xa0, 0xa0));
    }
    return p;
}

QString styleSheet(Mode mode)
{
    const bool dark = (mode == Mode::Dark);
    const QString win = dark ? QStringLiteral("#202020") : QStringLiteral("#f3f3f3");
    const QString text = dark ? QStringLiteral("#f0f0f0") : QStringLiteral("#1a1a1a");
    const QString hover = dark ? QStringLiteral("rgba(255,255,255,15)") : QStringLiteral("rgba(0,0,0,13)");
    const QString pressed = dark ? QStringLiteral("rgba(255,255,255,26)") : QStringLiteral("rgba(0,0,0,23)");
    const QString disabled = dark ? QStringLiteral("#8a8a8a") : QStringLiteral("#a0a0a0");
    const QString selBg = dark ? QStringLiteral("#2d2d2d") : QStringLiteral("#e4e4e4");
    const QString base = dark ? QStringLiteral("#2b2b2b") : QStringLiteral("#ffffff");
    const QString border = dark ? QStringLiteral("rgba(255,255,255,26)") : QStringLiteral("rgba(0,0,0,30)");
    const QString accentSoft = dark ? QStringLiteral("rgba(76,194,255,30)") : QStringLiteral("rgba(0,95,184,26)");
    const QString accent = dark ? QStringLiteral("#4cc2ff") : QStringLiteral("#005fb8");
    const QString menuBg = dark ? QStringLiteral("#2c2c2c") : QStringLiteral("#fafafa");
    const QString scroll = dark ? QStringLiteral("#4a4a4a") : QStringLiteral("#c8c8c8");
    const QString scrollHover = dark ? QStringLiteral("#5f5f5f") : QStringLiteral("#b0b0b0");
    const QString textDim = dark ? QStringLiteral("#b8b8b8") : QStringLiteral("#5c5c5c");
    const QString sepColor = dark ? QStringLiteral("#8a8a8a") : QStringLiteral("#6e6e6e");

    return QStringLiteral(R"(QWidget#root { background-color: %1; }
QWidget#titleBar, QWidget#commandBar, QWidget#statusBar, QWidget#hostPage { background: transparent; }
QToolButton { border: none; background: transparent; color: %2; }
QToolButton#nav { border-radius: 6px; padding: 4px; }
QToolButton#nav:hover { background: %3; }
QToolButton#nav:pressed { background: %4; }
QToolButton#nav:disabled { color: %5; }
QToolButton#cmd { border-radius: 6px; padding: 5px 10px; font-size: 12px; }
QToolButton#cmd:hover { background: %3; }
QToolButton#cmd:pressed { background: %4; }
QToolButton#cmd:disabled { color: %5; }
QToolButton#tabAdd { border-radius: 6px; margin: 2px; padding: 2px 8px; font-size: 14px; }
QToolButton#tabAdd:hover { background: %3; }
QTabBar { background: transparent; }
QTabBar::tab { background: transparent; color: %2; padding: 6px 12px; border-radius: 8px; margin: 3px 2px; }
QTabBar::tab:hover { background: %3; }
QTabBar::tab:selected { background: %6; }
QLineEdit#address, QLineEdit#search { background: %7; border: 1px solid %8; border-radius: 6px; padding: 4px 10px; color: %2; selection-background-color: %9; }
QLineEdit#address:focus, QLineEdit#search:focus { border: 1px solid %10; }
QListWidget#sidebar { background: transparent; border: none; outline: none; }
QListWidget#sidebar::item { padding: 6px 10px; border-radius: 6px; color: %2; }
QListWidget#sidebar::item:hover { background: %3; }
QListWidget#sidebar::item:selected { background: %6; color: %2; }
QMenu { background: %11; color: %2; border: 1px solid %8; border-radius: 8px; padding: 6px; }
QMenu::item { padding: 6px 26px; border-radius: 6px; }
QMenu::item:selected { background: %12; }
QMenu::item:disabled { color: %5; }
QMenu::separator { height: 1px; background: %8; margin: 6px 8px; }
QToolTip { background: %11; color: %2; border: 1px solid %8; }
QScrollBar:vertical { background: transparent; width: 12px; margin: 2px; }
QScrollBar::handle:vertical { background: %13; border-radius: 5px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: %14; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: transparent; height: 12px; margin: 2px; }
QScrollBar::handle:horizontal { background: %13; border-radius: 5px; min-width: 30px; }
QScrollBar::handle:horizontal:hover { background: %14; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QLabel#statusLabel { color: %15; }
QLabel#crumbSep { color: %16; }
QToolButton#crumb { border-radius: 6px; padding: 2px 8px; color: %2; }
QToolButton#crumb:hover { background: %17; }
QSplitter::handle { background: transparent; }
QToolButton#sidebarToggle, QToolButton#statusBtn { border-radius: 6px; padding: 3px; color: %15; }
QToolButton#sidebarToggle:hover, QToolButton#statusBtn:hover { background: %3; }
)")
        .arg(win)
        .arg(text)
        .arg(hover)
        .arg(pressed)
        .arg(disabled)
        .arg(selBg)
        .arg(base)
        .arg(border)
        .arg(accentSoft)
        .arg(accent)
        .arg(menuBg)
        .arg(accentSoft)
        .arg(scroll)
        .arg(scrollHover)
        .arg(textDim)
        .arg(sepColor)
        .arg(hover);
}

QIcon icon(const QString &primary, const QString &fallback)
{
    QIcon result = QIcon::fromTheme(primary);
    if (!result.isNull()) {
        return result;
    }
    if (!fallback.isEmpty()) {
        result = QIcon::fromTheme(fallback);
        if (!result.isNull()) {
            return result;
        }
    }
    return QIcon();
}

} // namespace Theme
