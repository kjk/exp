#pragma once

/// C++ port of GPUI's window module.
/// The Window manages rendering, layout, input dispatch, and focus for a single window.

#include <gpui/action.h>
#include <gpui/element.h>
#include <gpui/geometry.h>
#include <gpui/input.h>
#include <gpui/platform.h>
#include <gpui/scene.h>
#include <gpui/style.h>
#include <gpui/text_system.h>
#include <functional>
#include <memory>
#include <optional>
#include <stack>
#include <typeindex>
#include <vector>

namespace gpui {

// Forward declarations
class App;

// ─── Focus Handle ──────────────────────────────────────────────────────────

/// A handle to a focusable element.
struct FocusHandle {
    uint64_t id = 0;

    bool operator==(const FocusHandle& other) const { return id == other.id; }
    bool operator!=(const FocusHandle& other) const { return id != other.id; }
    explicit operator bool() const { return id != 0; }
};

// ─── Layout Engine ─────────────────────────────────────────────────────────

/// Abstract layout engine interface (wraps Taffy or similar).
class LayoutEngine {
public:
    virtual ~LayoutEngine() = default;

    /// Create a leaf node with a fixed size.
    virtual LayoutId create_leaf(const Style& style, Size<Pixels> size) = 0;

    /// Create a container node with children.
    virtual LayoutId create_node(const Style& style, const std::vector<LayoutId>& children) = 0;

    /// Create a node whose size is computed by a callback.
    virtual LayoutId create_measured(
        const Style& style,
        std::function<Size<Pixels>(Size<std::optional<Pixels>>)> measure) = 0;

    /// Compute the layout for a tree rooted at the given node.
    virtual void compute_layout(LayoutId root, Size<Pixels> available_space) = 0;

    /// Get the computed bounds for a layout node.
    virtual Bounds<Pixels> layout_bounds(LayoutId node) const = 0;

    /// Clear all layout nodes.
    virtual void clear() = 0;
};

/// Create a Taffy-based layout engine.
std::unique_ptr<LayoutEngine> create_layout_engine();

// ─── Dispatch Tree ─────────────────────────────────────────────────────────

/// A node in the dispatch tree for routing events.
struct DispatchNode {
    std::optional<FocusHandle> focus_handle;
    std::optional<ElementId> element_id;
    std::vector<std::function<bool(const InputEvent&)>> listeners;
    std::vector<std::function<void(const Action&)>> action_handlers;
};

// ─── Window ────────────────────────────────────────────────────────────────

/// The Window manages everything about a single application window:
/// layout computation, event dispatch, focus management, and rendering.
class Window {
public:
    Window(std::unique_ptr<PlatformWindow> platform_window,
           std::unique_ptr<LayoutEngine> layout_engine);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // ─── Layout ────────────────────────────────────────────────────────

    /// Request a layout node for a leaf element with a fixed size.
    LayoutId request_layout(const Style& style, Size<Pixels> size);

    /// Request a layout node for a container with children.
    LayoutId request_layout(const Style& style, const std::vector<LayoutId>& children);

    /// Request a layout node with a measure function.
    LayoutId request_measured_layout(
        const Style& style,
        std::function<Size<Pixels>(Size<std::optional<Pixels>>)> measure);

    /// Compute layout for the whole window.
    void compute_layout();

    /// Get the computed bounds for a layout node.
    Bounds<Pixels> layout_bounds(LayoutId id) const;

    // ─── Painting ──────────────────────────────────────────────────────

    /// Get the current scene being painted.
    Scene& scene() { return scene_; }
    const Scene& scene() const { return scene_; }

    /// Paint a quad (filled rectangle with optional border and corners).
    void paint_quad(Quad quad);

    /// Paint a shadow.
    void paint_shadow(Shadow shadow);

    /// Paint a glyph (monochrome sprite).
    void paint_glyph(MonochromeSprite sprite);

    /// Paint an image (polychrome sprite).
    void paint_image(PolychromeSprite sprite);

    /// Paint an underline.
    void paint_underline(Underline underline);

    /// Push a rendering layer (for z-ordering).
    void push_layer();

    /// Pop a rendering layer.
    void pop_layer();

    // ─── Content Mask (Clipping) ───────────────────────────────────────

    /// Push a content mask (clip region).
    void push_content_mask(ContentMask mask);

    /// Pop the current content mask.
    void pop_content_mask();

    /// Get the current content mask.
    std::optional<ContentMask> current_content_mask() const;

    // ─── Hitboxes ──────────────────────────────────────────────────────

    /// Register a hitbox for mouse event routing.
    HitboxId insert_hitbox(Bounds<Pixels> bounds, bool opaque = true);

    /// Test which hitbox is at a given point.
    std::optional<HitboxId> hit_test(Point<Pixels> point) const;

    // ─── Focus ─────────────────────────────────────────────────────────

    /// Create a new focus handle.
    FocusHandle create_focus_handle();

    /// Set focus to a handle.
    void focus(FocusHandle handle);

    /// Clear focus.
    void blur();

    /// Get the currently focused handle.
    std::optional<FocusHandle> focused() const { return focused_; }

    /// Check if a handle is focused.
    bool is_focused(FocusHandle handle) const {
        return focused_.has_value() && *focused_ == handle;
    }

    // ─── Input Events ──────────────────────────────────────────────────

    /// Register an event listener on the current dispatch node.
    void on_input(std::function<bool(const InputEvent&)> handler);

    /// Register a mouse down listener.
    void on_mouse_down(MouseButton button, std::function<void(const MouseDownEvent&)> handler);

    /// Register a mouse up listener.
    void on_mouse_up(MouseButton button, std::function<void(const MouseUpEvent&)> handler);

    /// Register a mouse move listener.
    void on_mouse_move(std::function<void(const MouseMoveEvent&)> handler);

    /// Register a scroll wheel listener.
    void on_scroll(std::function<void(const ScrollWheelEvent&)> handler);

    /// Register a key down listener.
    void on_key_down(std::function<void(const KeyDownEvent&)> handler);

    /// Register a key up listener.
    void on_key_up(std::function<void(const KeyUpEvent&)> handler);

    /// Register an action handler on the current dispatch node.
    void on_action(ActionTypeId type, std::function<void(const Action&)> handler);

    /// Typed action handler helper.
    template <typename A>
    void on_action(std::function<void(const A&)> handler) {
        on_action(std::type_index(typeid(A)), [h = std::move(handler)](const Action& a) {
            h(static_cast<const A&>(a));
        });
    }

    // ─── Dispatch ──────────────────────────────────────────────────────

    /// Dispatch an input event through the tree.
    bool dispatch_event(const InputEvent& event);

    /// Dispatch an action through the focus chain.
    bool dispatch_action(const Action& action);

    /// Push a dispatch node.
    void push_dispatch_node(std::optional<FocusHandle> focus = std::nullopt,
                            std::optional<ElementId> element_id = std::nullopt);

    /// Pop the current dispatch node.
    void pop_dispatch_node();

    // ─── Window State ──────────────────────────────────────────────────

    /// Get the window's content bounds.
    Bounds<Pixels> bounds() const;

    /// Get the window size.
    Size<Pixels> size() const;

    /// Get the display scale factor.
    float scale_factor() const;

    /// Get the current mouse position (in window coordinates).
    Point<Pixels> mouse_position() const { return mouse_position_; }

    /// Get the current modifier key state.
    Modifiers modifiers() const { return modifiers_; }

    /// Get the window appearance.
    WindowAppearance appearance() const;

    /// Invalidate the window (request a repaint).
    void invalidate();

    /// Set the window title.
    void set_title(const std::string& title);

    /// Get the text system.
    TextSystem& text_system();

    /// Get the underlying platform window.
    PlatformWindow& platform_window() { return *platform_window_; }

    /// Get the rem (root em) size in pixels.
    Pixels rem_size() const { return rem_size_; }

    /// Set the rem size.
    void set_rem_size(Pixels size) { rem_size_ = size; }

private:
    std::unique_ptr<PlatformWindow> platform_window_;
    std::unique_ptr<LayoutEngine> layout_engine_;
    Scene scene_;

    // Focus state
    std::optional<FocusHandle> focused_;
    uint64_t next_focus_id_ = 1;

    // Input state
    Point<Pixels> mouse_position_;
    Modifiers modifiers_;

    // Dispatch tree
    std::vector<DispatchNode> dispatch_stack_;

    // Content mask stack
    std::vector<ContentMask> content_mask_stack_;

    // Hitboxes
    std::vector<Hitbox> hitboxes_;
    uint64_t next_hitbox_id_ = 1;

    // Layout root
    std::optional<LayoutId> root_layout_;

    // Rem size
    Pixels rem_size_{16.0f};
};

} // namespace gpui
