cmake_minimum_required(VERSION 3.20)
project(ArchiveManager VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
find_package(KF6CoreAddons REQUIRED)
find_package(KF6Parts REQUIRED)
find_package(KF6XmlGui REQUIRED)
find_package(KF6KIO REQUIRED)

add_executable(archive-manager
    src/main.cpp
    src/theme.cpp
    src/breadcrumb.cpp
    src/sidebar.cpp
    src/shellwindow.cpp
)

target_link_libraries(archive-manager PRIVATE
    Qt6::Widgets
    KF6::CoreAddons
    KF6::Parts
    KF6::XmlGui
    KF6::KIOCore
)

install(TARGETS archive-manager RUNTIME DESTINATION bin)
install(FILES data/org.azendev.ArchiveManager.desktop DESTINATION share/applications)
