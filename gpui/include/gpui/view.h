#pragma once

/// C++ port of GPUI's view module.
/// Views are entities that implement Render, producing UI element trees.

#include <gpui/app.h>
#include <gpui/element.h>
#include <functional>
#include <memory>

namespace gpui {

// Forward declarations
class Window;

// ─── Render Trait ──────────────────────────────────────────────────────────

/// Concept for types that can render themselves as UI elements.
/// This is the core interface for views.
template <typename T>
concept Render = requires(T& t, Window& window, Context<T>& cx) {
    { t.render(window, cx) } -> IntoElement;
};

// ─── View ──────────────────────────────────────────────────────────────────

/// A View is an Entity that implements Render.
/// It produces a UI element tree when rendered and can be observed for changes.
template <typename V>
class View {
public:
    View() = default;
    explicit View(Entity<V> entity) : entity_(entity) {}

    /// Get the underlying entity.
    Entity<V>& entity() { return entity_; }
    const Entity<V>& entity() const { return entity_; }

    /// Read the view's state.
    const V& read(const App& cx) const { return entity_.read(cx); }

    /// Update the view's state.
    template <typename F>
    auto update(App& cx, F&& callback) {
        return entity_.update(cx, std::forward<F>(callback));
    }

    uint64_t id() const { return entity_.id(); }

    bool operator==(const View& other) const { return entity_ == other.entity_; }

private:
    Entity<V> entity_;
};

// ─── AnyView ───────────────────────────────────────────────────────────────

/// A type-erased view that can be rendered without knowing the concrete type.
class AnyView : public Element {
public:
    AnyView() = default;

    /// Create from a typed view.
    template <typename V>
        requires Render<V>
    explicit AnyView(View<V> view, App& cx)
        : entity_id_(view.id())
        , render_fn_(make_render_fn<V>(view)) {}

    std::optional<ElementId> id() const override {
        return ElementId(entity_id_);
    }

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
    using RenderFn = std::function<AnyElement(Window&, App&)>;

    template <typename V>
    static RenderFn make_render_fn(View<V> view) {
        return [view](Window& window, App& cx) -> AnyElement {
            return cx.update_entity(view.entity(), [&](V& state, Context<V>& ctx) {
                auto elem = state.render(window, ctx);
                return into_any_element(std::move(elem));
            });
        };
    }

    uint64_t entity_id_ = 0;
    RenderFn render_fn_;
    AnyElement rendered_;
};

// ─── View Construction ─────────────────────────────────────────────────────

/// Create a new view from an initial state.
template <typename V>
    requires Render<V>
View<V> new_view(App& cx, V state) {
    auto entity = cx.new_entity(std::move(state));
    return View<V>(entity);
}

/// Create a new view using an initializer callback.
template <typename V, typename F>
    requires Render<V>
View<V> new_view(App& cx, F&& initializer) {
    auto entity = cx.new_entity<V>(std::forward<F>(initializer));
    return View<V>(entity);
}

} // namespace gpui
