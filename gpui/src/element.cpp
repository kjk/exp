#include <gpui/element.h>
#include <gpui/window.h>

namespace gpui {

LayoutId Empty::request_layout(
    const std::optional<GlobalElementId>&,
    Window& window, App&) {
    Style style;
    return window.request_layout(style, Size<Pixels>::zero());
}

} // namespace gpui
