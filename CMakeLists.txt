cmake_minimum_required(VERSION 3.22)
project(pwaapp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Widgets WebEngineQuick)

qt_add_executable(pwaapp
    src/main.cpp
    src/site-classifier-bridge.hpp
    src/site-classifier-bridge.cpp
    src/app-installer-bridge.hpp
    src/app-installer-bridge.cpp
)

qt_add_qml_module(pwaapp
    URI PWAApp
    VERSION 1.0
    QML_FILES
        qml/main.qml
)

target_include_directories(pwaapp PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(pwaapp PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

# --- qapp-wrapper: standalone WebEngine launcher for installed apps ---
qt_add_executable(qapp-wrapper
    src/qapp-wrapper-main.cpp
)

qt_add_qml_module(qapp-wrapper
    URI QAppWrapper
    VERSION 1.0
    QML_FILES
        qml/wrapper.qml
)

target_link_libraries(qapp-wrapper PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Widgets
    Qt6::WebEngineQuick
)
