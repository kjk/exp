cmake_minimum_required(VERSION 3.20)
project(gpui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependency sub-libraries
add_subdirectory(collections)
add_subdirectory(refineable)
add_subdirectory(util)
add_subdirectory(scheduler)
add_subdirectory(http_client)

# GPUI core library
add_library(gpui
    src/geometry.cpp
    src/color.cpp
    src/scene.cpp
    src/style.cpp
    src/element.cpp
    src/input.cpp
    src/action.cpp
    src/executor.cpp
    src/text_system.cpp
    src/platform.cpp
    src/app.cpp
    src/window.cpp
    src/view.cpp
    src/div.cpp
    src/text_element.cpp
)

target_include_directories(gpui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

find_package(Threads REQUIRED)

target_link_libraries(gpui
    PUBLIC
        gpui_collections
        gpui_refineable
        gpui_util
        gpui_scheduler
        gpui_http_client
    PRIVATE
        Threads::Threads
)

# Platform-specific linking
if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(METAL_FRAMEWORK Metal)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)
    target_link_libraries(gpui PRIVATE
        ${COCOA_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
    )
elseif(WIN32)
    target_link_libraries(gpui PRIVATE d3d11 dxgi dwrite)
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 gtk+-3.0)
    if(GTK3_FOUND)
        target_include_directories(gpui PRIVATE ${GTK3_INCLUDE_DIRS})
        target_link_libraries(gpui PRIVATE ${GTK3_LIBRARIES})
    endif()
endif()
