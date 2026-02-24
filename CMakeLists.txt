cmake_minimum_required(VERSION 3.22)
project(qapp-framework VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Widgets WebEngineQuick Network DBus)

# --- qapp-common: shared static library (namespace qapp::) ---
qt_add_library(qapp-common STATIC
    common/cpp/qapp-common.cpp
    common/cpp/config-reader.cpp
    common/cpp/xdg-paths.cpp
    common/cpp/url-helpers.cpp
    common/cpp/desktop-entry.cpp
    common/cpp/app-metadata.cpp
    common/cpp/html-parser.cpp
    common/cpp/manifest-parser.cpp
    common/cpp/icon-manager.cpp
)

set_source_files_properties(common/qml/QAppAboutDialog.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/QAppAboutDialog.qml)
set_source_files_properties(common/qml/QAppDownloadHandler.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/QAppDownloadHandler.qml)
set_source_files_properties(common/qml/QAppDownloadBar.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/QAppDownloadBar.qml)
set_source_files_properties(common/qml/QAppResumeOverlay.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/QAppResumeOverlay.qml)

qt_add_qml_module(qapp-common
    URI QApp.Common
    VERSION 1.0
    QML_FILES
        common/qml/QAppAboutDialog.qml
        common/qml/QAppDownloadHandler.qml
        common/qml/QAppDownloadBar.qml
        common/qml/QAppResumeOverlay.qml
)

target_include_directories(qapp-common PUBLIC ${CMAKE_SOURCE_DIR}/common/cpp)

target_link_libraries(qapp-common PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

# --- qapp-installer: system UI (namespace qapp::installer::) ---
qt_add_executable(qapp-installer
    installer/cpp/main.cpp
    installer/cpp/site-classifier.hpp
    installer/cpp/site-classifier.cpp
    installer/cpp/classify-pipeline.hpp
    installer/cpp/classify-pipeline.cpp
    installer/cpp/app-manager.hpp
    installer/cpp/app-manager.cpp
    installer/cpp/web-fetcher.hpp
    installer/cpp/web-fetcher.cpp
)

# Strip binary subdir prefix from resource aliases so paths match main.cpp
set_source_files_properties(installer/qml/main.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/main.qml)
set_source_files_properties(installer/qml/InstallerInstallTab.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/InstallerInstallTab.qml)
set_source_files_properties(installer/qml/InstallerResultsPanel.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/InstallerResultsPanel.qml)
set_source_files_properties(installer/qml/InstallerManageTab.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/InstallerManageTab.qml)
set_source_files_properties(installer/qml/InstallerUpdateDialog.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/InstallerUpdateDialog.qml)

set_source_files_properties(assets/icons/qapp-dark-500.png
    PROPERTIES QT_RESOURCE_ALIAS icons/qapp-dark-500.png)
set_source_files_properties(assets/icons/qapp-top-dark-500.png
    PROPERTIES QT_RESOURCE_ALIAS icons/qapp-top-dark-500.png)

qt_add_qml_module(qapp-installer
    URI QApp.Installer
    VERSION 1.0
    QML_FILES
        installer/qml/main.qml
        installer/qml/InstallerInstallTab.qml
        installer/qml/InstallerResultsPanel.qml
        installer/qml/InstallerManageTab.qml
        installer/qml/InstallerUpdateDialog.qml
    RESOURCES
        assets/icons/qapp-dark-500.png
        assets/icons/qapp-top-dark-500.png
)

target_include_directories(qapp-installer PRIVATE ${CMAKE_SOURCE_DIR}/installer/cpp)

option(QAPP_DEV_BUILD "Mark as development build" OFF)
if(QAPP_DEV_BUILD)
    target_compile_definitions(qapp-installer PRIVATE QAPP_APP_NAME="QApp Dev" QAPP_APP_ID="qapp-installer-dev")
else()
    target_compile_definitions(qapp-installer PRIVATE QAPP_APP_NAME="QApp" QAPP_APP_ID="qapp-installer")
endif()

target_link_libraries(qapp-installer PRIVATE
    qapp-common
    qapp-commonplugin
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::WebEngineQuick
    Qt6::Network
)

# --- qapp-ws-wrapper: per-app shell for WS/WAPP (namespace qapp::ws::) ---
qt_add_executable(qapp-ws-wrapper
    ws-wrapper/cpp/qapp-ws-wrapper-main.cpp
    ws-wrapper/cpp/wrapper-helper.hpp
    ws-wrapper/cpp/wrapper-helper.cpp
)

target_include_directories(qapp-ws-wrapper PRIVATE ${CMAKE_SOURCE_DIR}/ws-wrapper/cpp)

set_source_files_properties(ws-wrapper/qml/wrapper.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/wrapper.qml)
set_source_files_properties(ws-wrapper/qml/WrapperTab.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/WrapperTab.qml)
set_source_files_properties(ws-wrapper/qml/WrapperHeader.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/WrapperHeader.qml)
set_source_files_properties(ws-wrapper/qml/WrapperTabView.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/WrapperTabView.qml)
set_source_files_properties(ws-wrapper/qml/WrapperSettingsDialog.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/WrapperSettingsDialog.qml)
set_source_files_properties(ws-wrapper/qml/WrapperSaveDialog.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/WrapperSaveDialog.qml)
set_source_files_properties(ws-wrapper/qml/WrapperFullscreenHint.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/WrapperFullscreenHint.qml)

qt_add_qml_module(qapp-ws-wrapper
    URI QApp.WS
    VERSION 1.0
    QML_FILES
        ws-wrapper/qml/wrapper.qml
        ws-wrapper/qml/WrapperTab.qml
        ws-wrapper/qml/WrapperHeader.qml
        ws-wrapper/qml/WrapperTabView.qml
        ws-wrapper/qml/WrapperSettingsDialog.qml
        ws-wrapper/qml/WrapperSaveDialog.qml
        ws-wrapper/qml/WrapperFullscreenHint.qml
)

if(QAPP_DEV_BUILD)
    target_compile_definitions(qapp-ws-wrapper PRIVATE QAPP_APP_NAME="QApp Dev" QAPP_DEV_BUILD)
else()
    target_compile_definitions(qapp-ws-wrapper PRIVATE QAPP_APP_NAME="QApp")
endif()

target_link_libraries(qapp-ws-wrapper PRIVATE
    qapp-common
    qapp-commonplugin
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Widgets
    Qt6::WebEngineQuick
)

# --- qapp-pwa-app: per-app shell for PWA (namespace qapp::pwa::) ---
qt_add_executable(qapp-pwa-app
    pwa-app/cpp/qapp-pwa-app-main.cpp
    pwa-app/cpp/pwa-helper.hpp
    pwa-app/cpp/pwa-helper.cpp
    pwa-app/cpp/pwa-permission-handler.hpp
    pwa-app/cpp/pwa-permission-handler.cpp
    pwa-app/cpp/pwa-notification-handler.hpp
    pwa-app/cpp/pwa-notification-handler.cpp
    pwa-app/cpp/pwa-os-integration.hpp
    pwa-app/cpp/pwa-os-integration.cpp
)

target_include_directories(qapp-pwa-app PRIVATE ${CMAKE_SOURCE_DIR}/pwa-app/cpp)

set_source_files_properties(pwa-app/qml/pwa-app.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/pwa-app.qml)
set_source_files_properties(pwa-app/qml/PwaAppMenu.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/PwaAppMenu.qml)
set_source_files_properties(pwa-app/qml/PwaFullscreenHint.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/PwaFullscreenHint.qml)
set_source_files_properties(pwa-app/qml/PwaSplashScreen.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/PwaSplashScreen.qml)
set_source_files_properties(pwa-app/qml/PwaToolbar.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/PwaToolbar.qml)
set_source_files_properties(pwa-app/qml/PwaOfflineIndicator.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/PwaOfflineIndicator.qml)

qt_add_qml_module(qapp-pwa-app
    URI QApp.PWA
    VERSION 1.0
    QML_FILES
        pwa-app/qml/pwa-app.qml
        pwa-app/qml/PwaAppMenu.qml
        pwa-app/qml/PwaFullscreenHint.qml
        pwa-app/qml/PwaSplashScreen.qml
        pwa-app/qml/PwaToolbar.qml
        pwa-app/qml/PwaOfflineIndicator.qml
)

if(QAPP_DEV_BUILD)
    target_compile_definitions(qapp-pwa-app PRIVATE QAPP_APP_NAME="QApp Dev" QAPP_DEV_BUILD)
else()
    target_compile_definitions(qapp-pwa-app PRIVATE QAPP_APP_NAME="QApp")
endif()

target_link_libraries(qapp-pwa-app PRIVATE
    qapp-common
    qapp-commonplugin
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Widgets
    Qt6::WebEngineQuick
    Qt6::Network
    Qt6::DBus
)

