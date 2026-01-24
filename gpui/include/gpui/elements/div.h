#pragma once

/// C++ port of GPUI's Div element.
/// A container element similar to HTML's <div>, supporting flex/grid layout,
/// styling, and event handling.

#include <gpui/action.h>
#include <gpui/element.h>
#include <gpui/input.h>
#include <gpui/styled.h>
#include <gpui/window.h>
#include <functional>
#include <optional>
#include <typeindex>
#include <vector>

namespace gpui {

/// A container element that lays out children using flexbox or grid.
/// Supports the full styled fluent API via CRTP.
class Div : public Element, public Styled<Div> {
public:
    Div() = default;
    Div(Div&&) noexcept = default;
    Div& operator=(Div&&) noexcept = default;
    Div(const Div&) = default;
    Div& operator=(const Div&) = default;

    // ─── Children ──────────────────────────────────────────────────────

    /// Add a child element.
    Div& child(AnyElement element) {
        children_.push_back(std::move(element));
        return *this;
    }

    /// Add a child that implements IntoElement.
    template <typename E>
        requires IntoElement<E>
    Div& child(E element) {
        children_.push_back(into_any_element(std::move(element)));
        return *this;
    }

    /// Add multiple children.
    Div& children(std::vector<AnyElement> elements) {
        for (auto& elem : elements) {
            children_.push_back(std::move(elem));
        }
        return *this;
    }

    /// Add children from an initializer.
    template <typename... Elements>
        requires (IntoElement<Elements> && ...)
    Div& children(Elements&&... elements) {
        (children_.push_back(into_any_element(std::forward<Elements>(elements))), ...);
        return *this;
    }

    // ─── Identity ──────────────────────────────────────────────────────

    /// Set the element ID (for state persistence across renders).
    Div& id(ElementId element_id) {
        id_ = std::move(element_id);
        return *this;
    }

    Div& id(const char* name) { return id(ElementId(std::string(name))); }
    Div& id(uint64_t n) { return id(ElementId(n)); }

    // ─── Focus ─────────────────────────────────────────────────────────

    /// Make this element focusable with the given focus handle.
    Div& focusable(FocusHandle handle) {
        focus_handle_ = handle;
        return *this;
    }

    // ─── Event Handlers ────────────────────────────────────────────────

    /// Handle mouse down events.
    Div& on_mouse_down(MouseButton button, std::function<void(const MouseDownEvent&, Window&, App&)> handler) {
        mouse_down_handlers_.push_back({button, std::move(handler)});
        return *this;
    }

    /// Handle mouse up events.
    Div& on_mouse_up(MouseButton button, std::function<void(const MouseUpEvent&, Window&, App&)> handler) {
        mouse_up_handlers_.push_back({button, std::move(handler)});
        return *this;
    }

    /// Handle click events (mouse down + mouse up on same element).
    Div& on_click(std::function<void(const ClickEvent&, Window&, App&)> handler) {
        click_handler_ = std::move(handler);
        return *this;
    }

    /// Handle mouse move events.
    Div& on_mouse_move(std::function<void(const MouseMoveEvent&, Window&, App&)> handler) {
        mouse_move_handler_ = std::move(handler);
        return *this;
    }

    /// Handle scroll events.
    Div& on_scroll(std::function<void(const ScrollWheelEvent&, Window&, App&)> handler) {
        scroll_handler_ = std::move(handler);
        return *this;
    }

    /// Handle key down events (requires focus).
    Div& on_key_down(std::function<void(const KeyDownEvent&, Window&, App&)> handler) {
        key_down_handler_ = std::move(handler);
        return *this;
    }

    /// Handle key up events (requires focus).
    Div& on_key_up(std::function<void(const KeyUpEvent&, Window&, App&)> handler) {
        key_up_handler_ = std::move(handler);
        return *this;
    }

    /// Handle an action (dispatched through the focus chain).
    template <typename A>
    Div& on_action(std::function<void(const A&, Window&, App&)> handler) {
        action_handlers_.push_back({
            std::type_index(typeid(A)),
            [h = std::move(handler)](const Action& a, Window& w, App& cx) {
                h(static_cast<const A&>(a), w, cx);
            }
        });
        return *this;
    }

    /// Handle hover state changes.
    Div& on_hover(std::function<void(bool hovered, Window&, App&)> handler) {
        hover_handler_ = std::move(handler);
        return *this;
    }

    // ─── Styled interface ──────────────────────────────────────────────

    StyleRefinement& style() { return style_; }
    const StyleRefinement& style() const { return style_; }

    // ─── Conditional styling ───────────────────────────────────────────

    /// Apply additional styling when hovered.
    Div& hover(std::function<void(StyleRefinement&)> style_fn) {
        hover_style_fn_ = std::move(style_fn);
        return *this;
    }

    /// Apply additional styling when active (pressed).
    Div& active(std::function<void(StyleRefinement&)> style_fn) {
        active_style_fn_ = std::move(style_fn);
        return *this;
    }

    // ─── Element interface ─────────────────────────────────────────────

    std::optional<ElementId> id() const override { return id_; }

    LayoutId request_layout(
        const std::optional<GlobalElementId>& global_id,
        Window& window, App& cx) override;

    void prepaint(
        const std::optional<GlobalElementId>& global_id,
        Bounds<Pixels> bounds, Window& window, App& cx) override;

    void paint(
        const std::optional<GlobalElementId>& global_id,
        Bounds<Pixels> bounds, Window& window, App& cx) override;

private:
    std::optional<ElementId> id_;
    StyleRefinement style_;
    std::vector<AnyElement> children_;
    std::optional<FocusHandle> focus_handle_;

    // Event handlers
    struct MouseButtonHandler {
        MouseButton button;
        std::function<void(const MouseDownEvent&, Window&, App&)> handler;
    };
    struct MouseUpButtonHandler {
        MouseButton button;
        std::function<void(const MouseUpEvent&, Window&, App&)> handler;
    };

    std::vector<MouseButtonHandler> mouse_down_handlers_;
    std::vector<MouseUpButtonHandler> mouse_up_handlers_;
    std::function<void(const ClickEvent&, Window&, App&)> click_handler_;
    std::function<void(const MouseMoveEvent&, Window&, App&)> mouse_move_handler_;
    std::function<void(const ScrollWheelEvent&, Window&, App&)> scroll_handler_;
    std::function<void(const KeyDownEvent&, Window&, App&)> key_down_handler_;
    std::function<void(const KeyUpEvent&, Window&, App&)> key_up_handler_;
    std::function<void(bool, Window&, App&)> hover_handler_;

    struct ActionHandler {
        std::type_index type;
        std::function<void(const Action&, Window&, App&)> handler;
    };
    std::vector<ActionHandler> action_handlers_;

    // Conditional styles
    std::function<void(StyleRefinement&)> hover_style_fn_;
    std::function<void(StyleRefinement&)> active_style_fn_;

    // State between phases
    std::vector<LayoutId> child_layout_ids_;
    std::optional<HitboxId> hitbox_id_;
};

/// Create a div element. This is the primary container element in GPUI.
inline Div div() { return Div{}; }

} // namespace gpui
