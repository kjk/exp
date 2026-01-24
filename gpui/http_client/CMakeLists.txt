add_library(gpui_http_client INTERFACE)

target_include_directories(gpui_http_client
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
