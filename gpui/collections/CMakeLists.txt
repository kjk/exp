add_library(gpui_collections INTERFACE)

target_include_directories(gpui_collections
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
