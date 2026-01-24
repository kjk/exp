#pragma once

/// C++ port of GPUI's element system.
/// Elements are the building blocks of the UI tree, with a three-phase
/// lifecycle: request_layout, prepaint, and paint.

#include <gpui/geometry.h>
#include <gpui/scene.h>
#include <gpui/style.h>
#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace gpui {

// Forward declarations
class Window;
class App;

// ─── Element ID ────────────────────────────────────────────────────────────

/// A unique identifier for an element within a view.
/// Can be an integer, a string name, or a focus handle.
struct ElementId {
    std::variant<uint64_t, std::string> id;

    ElementId() = default;
    explicit ElementId(uint64_t n) : id(n) {}
    explicit ElementId(std::string s) : id(std::move(s)) {}

    bool operator==(const ElementId& other) const { return id == other.id; }
    bool operator!=(const ElementId& other) const { return id != other.id; }
};

/// A globally unique element ID (path from root to element).
struct GlobalElementId {
    std::vector<ElementId> path;

    bool operator==(const GlobalElementId& other) const { return path == other.path; }
};

// ─── Layout ID ─────────────────────────────────────────────────────────────

/// An opaque handle to a layout node in the layout engine.
struct LayoutId {
    uint32_t value = 0;

    bool operator==(const LayoutId& other) const { return value == other.value; }
};

// ─── Element Trait ─────────────────────────────────────────────────────────

/// Base class for all UI elements.
/// Elements go through three phases:
/// 1. request_layout: Determine size requirements and create layout nodes.
/// 2. prepaint: Compute final positions after layout is resolved.
/// 3. paint: Emit rendering primitives to the scene.
class Element {
public:
    virtual ~Element() = default;

    /// Get the element's ID, if it has one.
    virtual std::optional<ElementId> id() const { return std::nullopt; }

    /// Request layout: create a layout node and return its ID.
    /// The element can store intermediate state for use in later phases.
    virtual LayoutId request_layout(
        const std::optional<GlobalElementId>& global_id,
        Window& window,
        App& cx) = 0;

    /// Prepaint phase: compute final bounds and prepare for painting.
    /// Called after the layout engine has resolved all positions.
    virtual void prepaint(
        const std::optional<GlobalElementId>& global_id,
        Bounds<Pixels> bounds,
        Window& window,
        App& cx) = 0;

    /// Paint phase: emit rendering primitives to the window's scene.
    virtual void paint(
        const std::optional<GlobalElementId>& global_id,
        Bounds<Pixels> bounds,
        Window& window,
        App& cx) = 0;
};

// ─── AnyElement ────────────────────────────────────────────────────────────

/// A type-erased element. Owns an element of any concrete type.
class AnyElement {
public:
    AnyElement() = default;

    // Move semantics preferred (elements are consumed during rendering).
    AnyElement(AnyElement&&) noexcept = default;
    AnyElement& operator=(AnyElement&&) noexcept = default;

    // Copy produces an empty element. Use std::move for ownership transfer.
    AnyElement(const AnyElement&) noexcept : element_(nullptr) {}
    AnyElement& operator=(const AnyElement&) noexcept { element_.reset(); return *this; }

    template <typename E>
        requires std::derived_from<E, Element>
    AnyElement(std::unique_ptr<E> element)
        : element_(std::move(element)) {}

    template <typename E>
        requires std::derived_from<E, Element>
    explicit AnyElement(E element)
        : element_(std::make_unique<E>(std::move(element))) {}

    bool has_value() const { return element_ != nullptr; }
    Element* get() { return element_.get(); }
    const Element* get() const { return element_.get(); }

    LayoutId request_layout(
        const std::optional<GlobalElementId>& global_id,
        Window& window, App& cx) {
        return element_->request_layout(global_id, window, cx);
    }

    void prepaint(
        const std::optional<GlobalElementId>& global_id,
        Bounds<Pixels> bounds, Window& window, App& cx) {
        element_->prepaint(global_id, bounds, window, cx);
    }

    void paint(
        const std::optional<GlobalElementId>& global_id,
        Bounds<Pixels> bounds, Window& window, App& cx) {
        element_->paint(global_id, bounds, window, cx);
    }

private:
    std::unique_ptr<Element> element_;
};

// ─── IntoElement ───────────────────────────────────────────────────────────

/// Concept for types that can be converted into an Element.
template <typename T>
concept IntoElement = requires(T t) {
    { t.into_element() } -> std::derived_from<Element>;
} || std::derived_from<T, Element>;

/// Convert a value into an AnyElement.
template <typename T>
    requires IntoElement<T>
AnyElement into_any_element(T value) {
    if constexpr (std::derived_from<T, Element>) {
        return AnyElement(std::make_unique<T>(std::move(value)));
    } else {
        auto elem = value.into_element();
        return AnyElement(std::make_unique<decltype(elem)>(std::move(elem)));
    }
}

// ─── RenderOnce ────────────────────────────────────────────────────────────

/// Concept for components that render once (stateless components).
/// They produce an element tree when rendered but don't maintain identity.
template <typename T>
concept RenderOnce = requires(T t, Window& window, App& cx) {
    { t.render(window, cx) } -> IntoElement;
};

// ─── Empty Element ─────────────────────────────────────────────────────────

/// An element that renders nothing. Useful as a placeholder.
class Empty : public Element {
public:
    LayoutId request_layout(
        const std::optional<GlobalElementId>&,
        Window& window, App& cx) override;

    void prepaint(
        const std::optional<GlobalElementId>&,
        Bounds<Pixels>, Window&, App&) override {}

    void paint(
        const std::optional<GlobalElementId>&,
        Bounds<Pixels>, Window&, App&) override {}
};

// ─── Element Utilities ─────────────────────────────────────────────────────

/// Helper to create a vector of child elements from a variadic list.
template <typename... Elements>
    requires (IntoElement<Elements> && ...)
std::vector<AnyElement> elements(Elements&&... elems) {
    std::vector<AnyElement> result;
    result.reserve(sizeof...(elems));
    (result.push_back(into_any_element(std::forward<Elements>(elems))), ...);
    return result;
}

} // namespace gpui
