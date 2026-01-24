#include <gpui/elements/div.h>
#include <gpui/app.h>
#include <gpui/window.h>

namespace gpui {

LayoutId Div::request_layout(
    const std::optional<GlobalElementId>& global_id,
    Window& window, App& cx) {

    // Resolve style to a concrete Style for layout
    Style resolved;
    resolved.apply(style_);

    // Request layout for all children
    child_layout_ids_.clear();
    child_layout_ids_.reserve(children_.size());

    for (auto& child : children_) {
        auto child_id = child.request_layout(global_id, window, cx);
        child_layout_ids_.push_back(child_id);
    }

    // Create a layout node with our style and children
    return window.request_layout(resolved, child_layout_ids_);
}

void Div::prepaint(
    const std::optional<GlobalElementId>& global_id,
    Bounds<Pixels> bounds, Window& window, App& cx) {

    // Register a hitbox for event handling
    if (click_handler_ || !mouse_down_handlers_.empty() ||
        !mouse_up_handlers_.empty() || hover_handler_) {
        hitbox_id_ = window.insert_hitbox(bounds);
    }

    // Push dispatch node for event routing
    window.push_dispatch_node(focus_handle_, id_);

    // Register event handlers
    if (!mouse_down_handlers_.empty()) {
        for (auto& handler : mouse_down_handlers_) {
            auto& h = handler;
            window.on_mouse_down(h.button, [&h, &window, &cx](const MouseDownEvent& e) {
                h.handler(e, window, cx);
            });
        }
    }

    if (key_down_handler_) {
        auto& handler = key_down_handler_;
        window.on_key_down([&handler, &window, &cx](const KeyDownEvent& e) {
            handler(e, window, cx);
        });
    }

    // Prepaint children
    for (std::size_t i = 0; i < children_.size(); ++i) {
        auto child_bounds = window.layout_bounds(child_layout_ids_[i]);
        children_[i].prepaint(global_id, child_bounds, window, cx);
    }

    window.pop_dispatch_node();
}

void Div::paint(
    const std::optional<GlobalElementId>& global_id,
    Bounds<Pixels> bounds, Window& window, App& cx) {

    Style resolved;
    resolved.apply(style_);

    // Paint shadows
    for (const auto& shadow_def : resolved.box_shadow) {
        Shadow shadow;
        shadow.bounds = bounds.dilate(shadow_def.spread_radius);
        shadow.bounds.origin = shadow.bounds.origin + shadow_def.offset;
        shadow.corner_radii = resolved.corner_radii;
        shadow.color = shadow_def.color;
        shadow.blur_radius = shadow_def.blur_radius;
        shadow.offset = shadow_def.offset;
        window.paint_shadow(std::move(shadow));
    }

    // Paint the quad (background + border)
    bool has_background = resolved.background.has_value();
    bool has_border = resolved.border_style != BorderStyle::None;

    if (has_background || has_border) {
        Quad quad;
        quad.bounds = bounds;
        if (has_background) {
            quad.background = *resolved.background;
        }
        quad.border_color = resolved.border_color;
        quad.border_widths = resolved.border_widths;
        quad.corner_radii = resolved.corner_radii;
        window.paint_quad(std::move(quad));
    }

    // Paint children
    window.push_layer();
    for (std::size_t i = 0; i < children_.size(); ++i) {
        auto child_bounds = window.layout_bounds(child_layout_ids_[i]);
        children_[i].paint(global_id, child_bounds, window, cx);
    }
    window.pop_layer();
}

} // namespace gpui
