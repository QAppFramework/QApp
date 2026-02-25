cmake_minimum_required(VERSION 3.24)
project(qapp-framework VERSION 0.1.0 LANGUAGES CXX)

# --- Build fingerprint from git ---
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE QAPP_GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
execute_process(
    COMMAND git diff --quiet HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE QAPP_GIT_DIRTY
)
if(QAPP_GIT_DIRTY)
    set(QAPP_GIT_SHA "${QAPP_GIT_SHA}-dirty")
endif()
if(NOT QAPP_GIT_SHA)
    set(QAPP_GIT_SHA "unknown")
endif()
message(STATUS "Build SHA: ${QAPP_GIT_SHA}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Gui Qml Quick QuickControls2 Widgets WebEngineQuick Network DBus QmlImportScanner)

# --- CXX-Qt CMake integration (lightweight cmake-only repo) ---
find_package(CxxQt QUIET)
if(NOT CxxQt_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        CxxQt
        GIT_REPOSITORY https://github.com/kdab/cxx-qt-cmake.git
        GIT_TAG 0.7.3
    )
    FetchContent_MakeAvailable(CxxQt)
endif()

# --- Import Rust crates via CXX-Qt + Corrosion ---
cxx_qt_import_crate(
    MANIFEST_PATH ${CMAKE_SOURCE_DIR}/Cargo.toml
    CRATES qapp_installer qapp_ws_wrapper qapp_pwa_app
    QT_MODULES Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
               Qt6::Network Qt6::Widgets Qt6::WebEngineQuick
)

# Import QML modules exported by each Rust crate
# Note: Corrosion normalizes hyphens to underscores in target names
cxx_qt_import_qml_module(qapp_installer_qml
    URI "QApp.Installer"
    SOURCE_CRATE qapp_installer)

cxx_qt_import_qml_module(qapp_ws_wrapper_qml
    URI "QApp.WS"
    SOURCE_CRATE qapp_ws_wrapper)

cxx_qt_import_qml_module(qapp_pwa_app_qml
    URI "QApp.PWA"
    SOURCE_CRATE qapp_pwa_app)

# --- qapp-common: shared QML module (pure QML, no Rust) ---
qt_add_library(qapp-common-qml STATIC)

set_source_files_properties(common/qml/QAppAboutDialog.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/QAppAboutDialog.qml)
set_source_files_properties(common/qml/QAppDownloadHandler.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/QAppDownloadHandler.qml)
set_source_files_properties(common/qml/QAppDownloadBar.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/QAppDownloadBar.qml)
set_source_files_properties(common/qml/QAppResumeOverlay.qml
    PROPERTIES QT_RESOURCE_ALIAS qml/QAppResumeOverlay.qml)

qt_add_qml_module(qapp-common-qml
    URI QApp.Common
    VERSION 1.0
    QML_FILES
        common/qml/QAppAboutDialog.qml
        common/qml/QAppDownloadHandler.qml
        common/qml/QAppDownloadBar.qml
        common/qml/QAppResumeOverlay.qml
)

target_link_libraries(qapp-common-qml PUBLIC Qt6::Core Qt6::Gui Qt6::Quick)

# --- qapp-qt-helpers: thin C++ wrappers around Qt APIs for Rust FFI ---
# OBJECT library ensures .o files are included directly (avoids static lib link order issues
# with CXX-generated code in Rust static libraries that reference these symbols)
add_library(qapp-qt-helpers OBJECT cpp/qt-helpers.cpp)
target_include_directories(qapp-qt-helpers PUBLIC ${CMAKE_SOURCE_DIR}/cpp)
target_link_libraries(qapp-qt-helpers PUBLIC Qt6::Core Qt6::DBus Qt6::Network Qt6::Gui Qt6::WebEngineQuick)

# --- Build option ---
option(QAPP_DEV_BUILD "Mark as development build" OFF)

# --- qapp-installer binary ---
add_executable(qapp-installer-exe cpp/installer-main.cpp)
set_target_properties(qapp-installer-exe PROPERTIES OUTPUT_NAME qapp-installer)

set_source_files_properties(assets/icons/qapp-dark-500.png
    PROPERTIES QT_RESOURCE_ALIAS icons/qapp-dark-500.png)
set_source_files_properties(assets/icons/qapp-top-dark-500.png
    PROPERTIES QT_RESOURCE_ALIAS icons/qapp-top-dark-500.png)

qt_add_resources(qapp-installer-exe "installer_assets"
    PREFIX "/QApp/Installer"
    FILES
        assets/icons/qapp-dark-500.png
        assets/icons/qapp-top-dark-500.png
)

if(QAPP_DEV_BUILD)
    target_compile_definitions(qapp-installer-exe PRIVATE
        QAPP_APP_NAME="QApp Dev" QAPP_APP_ID="qapp-installer-dev"
        QAPP_VERSION="${PROJECT_VERSION}" QAPP_BUILD_SHA="${QAPP_GIT_SHA}"
        QAPP_DEV_BUILD)
else()
    target_compile_definitions(qapp-installer-exe PRIVATE
        QAPP_APP_NAME="QApp" QAPP_APP_ID="qapp-installer"
        QAPP_VERSION="${PROJECT_VERSION}" QAPP_BUILD_SHA="${QAPP_GIT_SHA}")
endif()

target_link_libraries(qapp-installer-exe PRIVATE
    qapp_installer_qml
    qapp-common-qml
    qapp-common-qmlplugin
    qapp-qt-helpers
    Qt6::Widgets Qt6::WebEngineQuick Qt6::QuickControls2
)
qt_import_qml_plugins(qapp-installer-exe)

# --- qapp-ws-wrapper binary ---
add_executable(qapp-ws-wrapper-exe cpp/ws-wrapper-main.cpp)
set_target_properties(qapp-ws-wrapper-exe PROPERTIES OUTPUT_NAME qapp-ws-wrapper)

if(QAPP_DEV_BUILD)
    target_compile_definitions(qapp-ws-wrapper-exe PRIVATE
        QAPP_APP_NAME="QApp Dev" QAPP_VERSION="${PROJECT_VERSION}"
        QAPP_BUILD_SHA="${QAPP_GIT_SHA}" QAPP_DEV_BUILD)
else()
    target_compile_definitions(qapp-ws-wrapper-exe PRIVATE
        QAPP_APP_NAME="QApp" QAPP_VERSION="${PROJECT_VERSION}"
        QAPP_BUILD_SHA="${QAPP_GIT_SHA}")
endif()

target_link_libraries(qapp-ws-wrapper-exe PRIVATE
    qapp_ws_wrapper_qml
    qapp-common-qml
    qapp-common-qmlplugin
    qapp-qt-helpers
    Qt6::Widgets Qt6::WebEngineQuick
)
qt_import_qml_plugins(qapp-ws-wrapper-exe)

# --- qapp-pwa-app binary ---
add_executable(qapp-pwa-app-exe cpp/pwa-app-main.cpp)
set_target_properties(qapp-pwa-app-exe PROPERTIES OUTPUT_NAME qapp-pwa-app)

if(QAPP_DEV_BUILD)
    target_compile_definitions(qapp-pwa-app-exe PRIVATE
        QAPP_APP_NAME="QApp Dev" QAPP_VERSION="${PROJECT_VERSION}"
        QAPP_BUILD_SHA="${QAPP_GIT_SHA}" QAPP_DEV_BUILD)
else()
    target_compile_definitions(qapp-pwa-app-exe PRIVATE
        QAPP_APP_NAME="QApp" QAPP_VERSION="${PROJECT_VERSION}"
        QAPP_BUILD_SHA="${QAPP_GIT_SHA}")
endif()

target_link_libraries(qapp-pwa-app-exe PRIVATE
    qapp_pwa_app_qml
    qapp-common-qml
    qapp-common-qmlplugin
    qapp-qt-helpers
    Qt6::Widgets Qt6::WebEngineQuick Qt6::Network
)
qt_import_qml_plugins(qapp-pwa-app-exe)

# --- Install targets ---
install(TARGETS qapp-installer-exe qapp-ws-wrapper-exe qapp-pwa-app-exe
    RUNTIME DESTINATION bin)

install(PROGRAMS bin/update.sh
    DESTINATION share/qapp-framework)

install(FILES assets/icons/qapp.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME qapp-installer.svg)

install(FILES assets/qapp-installer.desktop
    DESTINATION share/applications)

# --- CPack packaging ---
set(CPACK_PACKAGE_NAME "qapp-framework")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Turn any website into a standalone desktop app")
set(CPACK_PACKAGE_CONTACT "QAppFramework <https://github.com/QAppFramework/QApp>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/QAppFramework/QApp")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# DEB
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgl1, libnss3, libxcomposite1, libxdamage1, libxrandr2, libxtst6")

# RPM
set(CPACK_RPM_PACKAGE_LICENSE "EUPL-1.2")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Internet")

include(CPack)
