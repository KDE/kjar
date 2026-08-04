# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

cmake_minimum_required(VERSION 3.28)
project(kjar VERSION 0.3.0)

set(QT_MIN_VERSION "6.11.0")
set(KF_MIN_VERSION "6.12.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMDeprecationSettings)
include(ECMFindQmlModule)
include(ECMSetupVersion)
include(FeatureSummary)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    I18n
    Kirigami
)

ecm_find_qmlmodule(org.kde.coreaddons REQUIRED)

qt_standard_project_setup(REQUIRES 6.11)

ecm_set_disabled_deprecation_versions(QT 6.11 KF 6.12)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KJAR
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kjar-version.h"
)

qt_add_executable(kjarapp
    src/main.cpp
    src/kjar.cpp
    src/kjar.h
    src/kjarapp.cpp
    src/kjarapp.h
)

qt_add_qml_module(kjarapp
    URI org.kde.kjar
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/ErrorDialog.qml
)

target_include_directories(kjarapp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(kjarapp PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    KF6::CoreAddons
    KF6::I18n
    KF6::I18nQml
    KF6::Kirigami
)

ki18n_install(po)

install(TARGETS kjarapp ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES org.kde.kjar.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kjar.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES icons/org.kde.kjar.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps
)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
