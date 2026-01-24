add_library(gpui_refineable INTERFACE)

target_include_directories(gpui_refineable
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
