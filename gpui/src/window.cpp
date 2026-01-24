#include <gpui/window.h>
#include <gpui/app.h>
#include <algorithm>

namespace gpui {

// ─── Simple Layout Engine ──────────────────────────────────────────────────
// A minimal layout engine implementation for testing.
// In production, this would be replaced by a Taffy port or yoga integration.

class SimpleLayoutEngine : public LayoutEngine {
public:
    struct Node {
        Style style;
        std::vector<LayoutId> children;
        Size<Pixels> fixed_size;
        std::function<Size<Pixels>(Size<std::optional<Pixels>>)> measure_fn;
        Bounds<Pixels> computed_bounds;
        bool is_leaf = false;
        bool is_measured = false;
    };

    LayoutId create_leaf(const Style& style, Size<Pixels> size) override {
        LayoutId id{static_cast<uint32_t>(nodes_.size())};
        Node node;
        node.style = style;
        node.fixed_size = size;
        node.is_leaf = true;
        nodes_.push_back(std::move(node));
        return id;
    }

    LayoutId create_node(const Style& style, const std::vector<LayoutId>& children) override {
        LayoutId id{static_cast<uint32_t>(nodes_.size())};
        Node node;
        node.style = style;
        node.children = children;
        nodes_.push_back(std::move(node));
        return id;
    }

    LayoutId create_measured(
        const Style& style,
        std::function<Size<Pixels>(Size<std::optional<Pixels>>)> measure) override {
        LayoutId id{static_cast<uint32_t>(nodes_.size())};
        Node node;
        node.style = style;
        node.measure_fn = std::move(measure);
        node.is_measured = true;
        nodes_.push_back(std::move(node));
        return id;
    }

    void compute_layout(LayoutId root, Size<Pixels> available_space) override {
        if (root.value >= nodes_.size()) return;
        layout_node(root, available_space, Point<Pixels>::zero());
    }

    Bounds<Pixels> layout_bounds(LayoutId node) const override {
        if (node.value >= nodes_.size()) return Bounds<Pixels>::zero();
        return nodes_[node.value].computed_bounds;
    }

    void clear() override {
        nodes_.clear();
    }

private:
    void layout_node(LayoutId id, Size<Pixels> available, Point<Pixels> origin) {
        auto& node = nodes_[id.value];

        if (node.is_leaf) {
            node.computed_bounds = Bounds<Pixels>{origin, node.fixed_size};
            return;
        }

        if (node.is_measured && node.measure_fn) {
            auto size = node.measure_fn(Size<std::optional<Pixels>>{
                std::optional<Pixels>(available.width),
                std::optional<Pixels>(available.height)
            });
            node.computed_bounds = Bounds<Pixels>{origin, size};
            return;
        }

        // Simple flex layout (column direction by default)
        bool is_row = node.style.flex_direction == FlexDirection::Row ||
                      node.style.flex_direction == FlexDirection::RowReverse;

        Pixels main_offset{0.0f};
        Pixels cross_max{0.0f};

        for (auto& child_id : node.children) {
            auto& child = nodes_[child_id.value];
            Size<Pixels> child_available = available;

            Point<Pixels> child_origin = origin;
            if (is_row) {
                child_origin.x = child_origin.x + main_offset;
            } else {
                child_origin.y = child_origin.y + main_offset;
            }

            layout_node(child_id, child_available, child_origin);

            auto child_size = child.computed_bounds.size;
            if (is_row) {
                main_offset += child_size.width;
                if (child_size.height > cross_max) cross_max = child_size.height;
            } else {
                main_offset += child_size.height;
                if (child_size.width > cross_max) cross_max = child_size.width;
            }
        }

        Size<Pixels> total_size;
        if (is_row) {
            total_size = Size<Pixels>{main_offset, cross_max};
        } else {
            total_size = Size<Pixels>{cross_max, main_offset};
        }

        // Apply explicit size if set
        // (simplified - doesn't handle all Length variants)
        node.computed_bounds = Bounds<Pixels>{origin, total_size};
    }

    std::vector<Node> nodes_;
};

std::unique_ptr<LayoutEngine> create_layout_engine() {
    return std::make_unique<SimpleLayoutEngine>();
}

// ─── Window Implementation ─────────────────────────────────────────────────

Window::Window(std::unique_ptr<PlatformWindow> platform_window,
               std::unique_ptr<LayoutEngine> layout_engine)
    : platform_window_(std::move(platform_window))
    , layout_engine_(std::move(layout_engine)) {}

Window::~Window() = default;

// Layout

LayoutId Window::request_layout(const Style& style, Size<Pixels> size) {
    return layout_engine_->create_leaf(style, size);
}

LayoutId Window::request_layout(const Style& style, const std::vector<LayoutId>& children) {
    return layout_engine_->create_node(style, children);
}

LayoutId Window::request_measured_layout(
    const Style& style,
    std::function<Size<Pixels>(Size<std::optional<Pixels>>)> measure) {
    return layout_engine_->create_measured(style, std::move(measure));
}

void Window::compute_layout() {
    if (root_layout_) {
        layout_engine_->compute_layout(*root_layout_, size());
    }
}

Bounds<Pixels> Window::layout_bounds(LayoutId id) const {
    return layout_engine_->layout_bounds(id);
}

// Painting

void Window::paint_quad(Quad quad) {
    scene_.insert_quad(std::move(quad));
}

void Window::paint_shadow(Shadow shadow) {
    scene_.insert_shadow(std::move(shadow));
}

void Window::paint_glyph(MonochromeSprite sprite) {
    scene_.insert_monochrome_sprite(std::move(sprite));
}

void Window::paint_image(PolychromeSprite sprite) {
    scene_.insert_polychrome_sprite(std::move(sprite));
}

void Window::paint_underline(Underline underline) {
    scene_.insert_underline(std::move(underline));
}

void Window::push_layer() {
    scene_.push_layer();
}

void Window::pop_layer() {
    scene_.pop_layer();
}

// Content Mask

void Window::push_content_mask(ContentMask mask) {
    content_mask_stack_.push_back(std::move(mask));
}

void Window::pop_content_mask() {
    if (!content_mask_stack_.empty()) {
        content_mask_stack_.pop_back();
    }
}

std::optional<ContentMask> Window::current_content_mask() const {
    if (content_mask_stack_.empty()) return std::nullopt;
    return content_mask_stack_.back();
}

// Hitboxes

HitboxId Window::insert_hitbox(Bounds<Pixels> bounds, bool opaque) {
    HitboxId id{next_hitbox_id_++};
    ContentMask mask;
    if (!content_mask_stack_.empty()) {
        mask = content_mask_stack_.back();
    } else {
        mask.bounds = Bounds<Pixels>{Point<Pixels>::zero(),
                                     Size<Pixels>{Pixels::max(), Pixels::max()}};
    }
    hitboxes_.push_back(Hitbox{id, bounds, mask, opaque});
    return id;
}

std::optional<HitboxId> Window::hit_test(Point<Pixels> point) const {
    // Walk hitboxes in reverse order (top-most first)
    for (auto it = hitboxes_.rbegin(); it != hitboxes_.rend(); ++it) {
        if (it->bounds.contains(point) && it->mask.bounds.contains(point)) {
            return it->id;
        }
    }
    return std::nullopt;
}

// Focus

FocusHandle Window::create_focus_handle() {
    return FocusHandle{next_focus_id_++};
}

void Window::focus(FocusHandle handle) {
    focused_ = handle;
}

void Window::blur() {
    focused_ = std::nullopt;
}

// Input events

void Window::on_input(std::function<bool(const InputEvent&)> handler) {
    if (!dispatch_stack_.empty()) {
        dispatch_stack_.back().listeners.push_back(std::move(handler));
    }
}

void Window::on_mouse_down(MouseButton button,
                            std::function<void(const MouseDownEvent&)> handler) {
    on_input([button, h = std::move(handler)](const InputEvent& event) -> bool {
        if (auto* e = std::get_if<MouseDownEvent>(&event)) {
            if (e->button == button) {
                h(*e);
                return true;
            }
        }
        return false;
    });
}

void Window::on_mouse_up(MouseButton button,
                           std::function<void(const MouseUpEvent&)> handler) {
    on_input([button, h = std::move(handler)](const InputEvent& event) -> bool {
        if (auto* e = std::get_if<MouseUpEvent>(&event)) {
            if (e->button == button) {
                h(*e);
                return true;
            }
        }
        return false;
    });
}

void Window::on_mouse_move(std::function<void(const MouseMoveEvent&)> handler) {
    on_input([h = std::move(handler)](const InputEvent& event) -> bool {
        if (auto* e = std::get_if<MouseMoveEvent>(&event)) {
            h(*e);
            return true;
        }
        return false;
    });
}

void Window::on_scroll(std::function<void(const ScrollWheelEvent&)> handler) {
    on_input([h = std::move(handler)](const InputEvent& event) -> bool {
        if (auto* e = std::get_if<ScrollWheelEvent>(&event)) {
            h(*e);
            return true;
        }
        return false;
    });
}

void Window::on_key_down(std::function<void(const KeyDownEvent&)> handler) {
    on_input([h = std::move(handler)](const InputEvent& event) -> bool {
        if (auto* e = std::get_if<KeyDownEvent>(&event)) {
            h(*e);
            return true;
        }
        return false;
    });
}

void Window::on_key_up(std::function<void(const KeyUpEvent&)> handler) {
    on_input([h = std::move(handler)](const InputEvent& event) -> bool {
        if (auto* e = std::get_if<KeyUpEvent>(&event)) {
            h(*e);
            return true;
        }
        return false;
    });
}

void Window::on_action(ActionTypeId type, std::function<void(const Action&)> handler) {
    if (!dispatch_stack_.empty()) {
        dispatch_stack_.back().action_handlers.push_back(std::move(handler));
    }
}

// Dispatch

bool Window::dispatch_event(const InputEvent& event) {
    // Try dispatching through all nodes in the dispatch stack (bubble phase)
    for (auto it = dispatch_stack_.rbegin(); it != dispatch_stack_.rend(); ++it) {
        for (auto& listener : it->listeners) {
            if (listener(event)) {
                return true;
            }
        }
    }
    return false;
}

bool Window::dispatch_action(const Action& action) {
    for (auto it = dispatch_stack_.rbegin(); it != dispatch_stack_.rend(); ++it) {
        for (auto& handler : it->action_handlers) {
            handler(action);
            return true;  // Action handled
        }
    }
    return false;
}

void Window::push_dispatch_node(std::optional<FocusHandle> focus_handle,
                                 std::optional<ElementId> element_id) {
    dispatch_stack_.push_back(DispatchNode{focus_handle, element_id, {}, {}});
}

void Window::pop_dispatch_node() {
    if (!dispatch_stack_.empty()) {
        dispatch_stack_.pop_back();
    }
}

// Window state

Bounds<Pixels> Window::bounds() const {
    return platform_window_->bounds();
}

Size<Pixels> Window::size() const {
    return platform_window_->bounds().size;
}

float Window::scale_factor() const {
    return platform_window_->scale_factor();
}

WindowAppearance Window::appearance() const {
    return platform_window_->appearance();
}

void Window::invalidate() {
    platform_window_->invalidate();
}

void Window::set_title(const std::string& title) {
    platform_window_->set_title(title);
}

TextSystem& Window::text_system() {
    // TODO: Get from app/platform
    static auto ts = create_text_system();
    return *ts;
}

} // namespace gpui
