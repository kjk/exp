add_library(gpui_scheduler INTERFACE)

target_include_directories(gpui_scheduler
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
