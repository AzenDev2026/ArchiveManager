cmake_minimum_required(VERSION 3.20)
project(DolphinShell VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
find_package(KF6CoreAddons REQUIRED)
find_package(KF6Parts REQUIRED)
find_package(KF6XmlGui REQUIRED)
find_package(KF6KIO REQUIRED)

add_executable(dolphin-shell
    src/main.cpp
    src/theme.cpp
    src/windowcontrols.cpp
    src/breadcrumb.cpp
    src/sidebar.cpp
    src/shellwindow.cpp
    resources/resources.qrc
)

target_link_libraries(dolphin-shell PRIVATE
    Qt6::Widgets
    KF6::CoreAddons
    KF6::Parts
    KF6::XmlGui
    KF6::KIOCore
)

install(TARGETS dolphin-shell RUNTIME DESTINATION bin)
install(FILES resources/dolphin-shell.svg DESTINATION share/icons/hicolor/scalable/apps)
install(FILES data/org.azendev.DolphinShell.desktop DESTINATION share/applications)
