/*
    Dolphin Shell — a Windows 11 Explorer style shell for Dolphin.
    SPDX-License-Identifier: MIT
*/
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLocale>
#include <QTimer>
#include <QUrl>

#include "shellwindow.h"

int main(int argc, char **argv)
{
    qputenv("LANGUAGE", "en");
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Dolphin Shell"));
    QApplication::setApplicationDisplayName(QStringLiteral("Dolphin Shell"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setOrganizationName(QStringLiteral("AzenDev"));
    QApplication::setDesktopFileName(QStringLiteral("org.azendev.DolphinShell"));
    QApplication::setStyle(QStringLiteral("Fusion"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("A Windows 11 Explorer style shell for Dolphin."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption screenshotOpt(QStringLiteral("screenshot"),
                                     QStringLiteral("Save a screenshot of the main window and exit."),
                                     QStringLiteral("file"));
    QCommandLineOption dumpOpt(QStringLiteral("dump-actions"),
                               QStringLiteral("Print the actions provided by the Dolphin engine and exit."));
    QCommandLineOption urlOpt(QStringLiteral("url"),
                              QStringLiteral("Open this location at startup."),
                              QStringLiteral("url"));
    QCommandLineOption themeOpt(QStringLiteral("theme"),
                                QStringLiteral("Start with a specific theme (dark or light)."),
                                QStringLiteral("theme"));
    parser.addOption(screenshotOpt);
    parser.addOption(dumpOpt);
    parser.addOption(urlOpt);
    parser.addOption(themeOpt);
    parser.process(app);

    ShellWindow window(nullptr, parser.isSet(themeOpt) ? parser.value(themeOpt) : QString());
    window.show();

    if (parser.isSet(urlOpt)) {
        window.openInitialUrl(QUrl::fromUserInput(parser.value(urlOpt), QDir::homePath()));
    }
    if (parser.isSet(screenshotOpt)) {
        const QString file = parser.value(screenshotOpt);
        QTimer::singleShot(1500, &window, [&window, file]() {
            window.grab().save(file);
            QTimer::singleShot(300, qApp, &QCoreApplication::quit);
        });
    }
    if (parser.isSet(dumpOpt)) {
        window.dumpActionsAndQuit();
    }

    return app.exec();
}
