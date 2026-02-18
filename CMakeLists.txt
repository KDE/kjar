# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

cmake_minimum_required(VERSION 3.28)
project(KjarApp)

set(QT_MIN_VERSION "6.9.0")
set(KF_MIN_VERSION "6.12.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Qml
    Quick
    QuickControls2
    Widgets
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    I18n
    Kirigami
    Notifications
)

qt6_add_resources(kjarapp_RESOURCES resources.qrc)

set(SOURCES
    src/main.cpp
    src/kjarapp.cpp
    ${kjarapp_RESOURCES}
)

add_executable(kjarapp ${SOURCES})

qt6_add_resources(kjar_watcher_RESOURCES
    src/watcher/watcher.qrc
)

set(SOURCES
    src/main.cpp
    src/kjarapp.cpp
    ${kjarapp_RESOURCES}
)

add_executable(kjar_watcher
    src/watcher/kjar_watcher.cpp
    ${kjar_watcher_RESOURCES}
)

target_link_libraries(kjarapp
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    KF6::CoreAddons
    KF6::I18n
    KF6::Kirigami
)

target_link_libraries(kjar_watcher
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    KF6::Kirigami
    KF6::I18n
)

install(TARGETS kjarapp kjar_watcher ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

