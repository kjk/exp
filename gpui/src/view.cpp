#include <gpui/view.h>
#include <gpui/window.h>

namespace gpui {

LayoutId AnyView::request_layout(
    const std::optional<GlobalElementId>& global_id,
    Window& window, App& cx) {
    if (render_fn_) {
        rendered_ = render_fn_(window, cx);
        if (rendered_.has_value()) {
            return rendered_.request_layout(global_id, window, cx);
        }
    }
    // Return a zero-size layout if nothing to render
    Style style;
    return window.request_layout(style, Size<Pixels>::zero());
}

void AnyView::prepaint(
    const std::optional<GlobalElementId>& global_id,
    Bounds<Pixels> bounds, Window& window, App& cx) {
    if (rendered_.has_value()) {
        rendered_.prepaint(global_id, bounds, window, cx);
    }
}

void AnyView::paint(
    const std::optional<GlobalElementId>& global_id,
    Bounds<Pixels> bounds, Window& window, App& cx) {
    if (rendered_.has_value()) {
        rendered_.paint(global_id, bounds, window, cx);
    }
}

} // namespace gpui
