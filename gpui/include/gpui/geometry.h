#pragma once

/// C++ port of GPUI's geometry module.
/// Provides 2D geometric primitives with type-safe measurement units.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace gpui {

// ─── Measurement Units ─────────────────────────────────────────────────────

/// Logical pixels - the base UI coordinate unit (resolution-independent).
struct Pixels {
    float value = 0.0f;

    constexpr Pixels() = default;
    constexpr explicit Pixels(float v) : value(v) {}

    constexpr Pixels operator+(Pixels other) const { return Pixels{value + other.value}; }
    constexpr Pixels operator-(Pixels other) const { return Pixels{value - other.value}; }
    constexpr Pixels operator*(float s) const { return Pixels{value * s}; }
    constexpr Pixels operator/(float s) const { return Pixels{value / s}; }
    constexpr Pixels operator-() const { return Pixels{-value}; }

    constexpr Pixels& operator+=(Pixels other) { value += other.value; return *this; }
    constexpr Pixels& operator-=(Pixels other) { value -= other.value; return *this; }
    constexpr Pixels& operator*=(float s) { value *= s; return *this; }

    constexpr bool operator==(Pixels other) const { return value == other.value; }
    constexpr bool operator!=(Pixels other) const { return value != other.value; }
    constexpr bool operator<(Pixels other) const { return value < other.value; }
    constexpr bool operator<=(Pixels other) const { return value <= other.value; }
    constexpr bool operator>(Pixels other) const { return value > other.value; }
    constexpr bool operator>=(Pixels other) const { return value >= other.value; }

    constexpr bool is_zero() const { return value == 0.0f; }
    constexpr Pixels half() const { return Pixels{value * 0.5f}; }
    constexpr Pixels abs() const { return Pixels{std::abs(value)}; }

    Pixels round() const { return Pixels{std::round(value)}; }
    Pixels ceil() const { return Pixels{std::ceil(value)}; }
    Pixels floor() const { return Pixels{std::floor(value)}; }

    static constexpr Pixels zero() { return Pixels{0.0f}; }
    static constexpr Pixels max() { return Pixels{std::numeric_limits<float>::max()}; }
};

constexpr Pixels operator*(float s, Pixels p) { return Pixels{s * p.value}; }

/// Physical device pixels (accounts for display scaling).
struct DevicePixels {
    int32_t value = 0;

    constexpr DevicePixels() = default;
    constexpr explicit DevicePixels(int32_t v) : value(v) {}

    constexpr DevicePixels operator+(DevicePixels other) const { return DevicePixels{value + other.value}; }
    constexpr DevicePixels operator-(DevicePixels other) const { return DevicePixels{value - other.value}; }
    constexpr DevicePixels operator*(int32_t s) const { return DevicePixels{value * s}; }

    constexpr bool operator==(DevicePixels other) const { return value == other.value; }
    constexpr bool operator<(DevicePixels other) const { return value < other.value; }
};

/// Scaled pixels - intermediate between logical and device pixels.
struct ScaledPixels {
    float value = 0.0f;

    constexpr ScaledPixels() = default;
    constexpr explicit ScaledPixels(float v) : value(v) {}

    constexpr ScaledPixels operator+(ScaledPixels other) const { return ScaledPixels{value + other.value}; }
    constexpr ScaledPixels operator-(ScaledPixels other) const { return ScaledPixels{value - other.value}; }
    constexpr ScaledPixels operator*(float s) const { return ScaledPixels{value * s}; }
};

/// Relative units based on the root font size.
struct Rems {
    float value = 0.0f;

    constexpr Rems() = default;
    constexpr explicit Rems(float v) : value(v) {}

    constexpr Rems operator+(Rems other) const { return Rems{value + other.value}; }
    constexpr Rems operator*(float s) const { return Rems{value * s}; }

    /// Convert to pixels given a base font size.
    constexpr Pixels to_pixels(Pixels base_font_size) const {
        return Pixels{value * base_font_size.value};
    }
};

/// An absolute length: either pixels or rems.
struct AbsoluteLength {
    enum class Tag { Pixels, Rems };

    Tag tag = Tag::Pixels;
    union {
        Pixels pixels;
        gpui::Rems rems;
    };

    constexpr AbsoluteLength() : tag(Tag::Pixels), pixels{} {}
    constexpr AbsoluteLength(Pixels px) : tag(Tag::Pixels), pixels(px) {}
    constexpr AbsoluteLength(gpui::Rems r) : tag(Tag::Rems), rems(r) {}

    Pixels to_pixels(Pixels rem_size) const {
        switch (tag) {
            case Tag::Pixels: return pixels;
            case Tag::Rems: return rems.to_pixels(rem_size);
        }
        return pixels;
    }
};

/// A definite length: absolute, fraction, or auto.
struct DefiniteLength {
    enum class Tag { Absolute, Fraction };

    Tag tag = Tag::Absolute;
    union {
        AbsoluteLength absolute;
        float fraction;
    };

    constexpr DefiniteLength() : tag(Tag::Absolute), absolute{} {}
    constexpr DefiniteLength(AbsoluteLength abs) : tag(Tag::Absolute), absolute(abs) {}
    constexpr DefiniteLength(Pixels px) : tag(Tag::Absolute), absolute(px) {}
    constexpr DefiniteLength(Rems r) : tag(Tag::Absolute), absolute(r) {}

    static constexpr DefiniteLength from_fraction(float f) {
        DefiniteLength d;
        d.tag = Tag::Fraction;
        d.fraction = f;
        return d;
    }
};

/// A length that can be definite or auto.
struct Length {
    enum class Tag { Definite, Auto };

    Tag tag = Tag::Auto;
    DefiniteLength definite{};

    constexpr Length() = default;
    constexpr Length(DefiniteLength d) : tag(Tag::Definite), definite(d) {}
    constexpr Length(Pixels px) : tag(Tag::Definite), definite(px) {}
    constexpr Length(Rems r) : tag(Tag::Definite), definite(r) {}

    static constexpr Length auto_() { return Length{}; }

    constexpr bool is_auto() const { return tag == Tag::Auto; }
};

// ─── Axis ──────────────────────────────────────────────────────────────────

/// Layout axis direction.
enum class Axis {
    Horizontal,
    Vertical,
};

/// Named corners.
enum class Corner {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
};

// ─── Point ─────────────────────────────────────────────────────────────────

/// A 2D point with x and y coordinates.
template <typename T>
struct Point {
    T x{};
    T y{};

    constexpr Point() = default;
    constexpr Point(T x, T y) : x(x), y(y) {}

    constexpr Point operator+(const Point& other) const {
        return Point{x + other.x, y + other.y};
    }
    constexpr Point operator-(const Point& other) const {
        return Point{x - other.x, y - other.y};
    }

    constexpr Point& operator+=(const Point& other) {
        x = x + other.x;
        y = y + other.y;
        return *this;
    }

    constexpr bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }

    /// Scale both components by a scalar.
    template <typename S>
    constexpr Point scale(S s) const {
        return Point{x * s, y * s};
    }

    /// Get the component along the given axis.
    constexpr T along(Axis axis) const {
        return axis == Axis::Horizontal ? x : y;
    }

    /// Apply a function to both components.
    template <typename F>
    auto map(F&& f) const {
        using R = decltype(f(x));
        return Point<R>{f(x), f(y)};
    }

    /// Component-wise min.
    constexpr Point min(const Point& other) const {
        return Point{std::min(x, other.x), std::min(y, other.y)};
    }

    /// Component-wise max.
    constexpr Point max(const Point& other) const {
        return Point{std::max(x, other.x), std::max(y, other.y)};
    }

    /// Clamp to a range.
    constexpr Point clamp(const Point& min_val, const Point& max_val) const {
        return Point{
            std::max(min_val.x, std::min(x, max_val.x)),
            std::max(min_val.y, std::min(y, max_val.y))
        };
    }

    static constexpr Point zero() { return Point{T{}, T{}}; }
};

// ─── Size ──────────────────────────────────────────────────────────────────

/// A 2D size with width and height.
template <typename T>
struct Size {
    T width{};
    T height{};

    constexpr Size() = default;
    constexpr Size(T w, T h) : width(w), height(h) {}

    constexpr bool operator==(const Size& other) const {
        return width == other.width && height == other.height;
    }

    /// Get the component along the given axis.
    constexpr T along(Axis axis) const {
        return axis == Axis::Horizontal ? width : height;
    }

    /// Scale both components.
    template <typename S>
    constexpr Size scale(S s) const {
        return Size{width * s, height * s};
    }

    /// Compute the center point.
    constexpr Point<T> center() const {
        return Point<T>{width / T{2}, height / T{2}};
    }

    /// Apply a function to both components.
    template <typename F>
    auto map(F&& f) const {
        using R = decltype(f(width));
        return Size<R>{f(width), f(height)};
    }

    static constexpr Size zero() { return Size{T{}, T{}}; }
};

// ─── Bounds ────────────────────────────────────────────────────────────────

/// A rectangle defined by an origin point and a size.
template <typename T>
struct Bounds {
    Point<T> origin;
    Size<T> size;

    constexpr Bounds() = default;
    constexpr Bounds(Point<T> origin, Size<T> size) : origin(origin), size(size) {}

    constexpr bool operator==(const Bounds& other) const {
        return origin == other.origin && size == other.size;
    }

    /// The right edge (origin.x + width).
    constexpr T right() const { return origin.x + size.width; }

    /// The bottom edge (origin.y + height).
    constexpr T bottom() const { return origin.y + size.height; }

    /// Center point of the bounds.
    constexpr Point<T> center() const {
        return Point<T>{
            origin.x + size.width / T{2},
            origin.y + size.height / T{2}
        };
    }

    /// Top-left corner.
    constexpr Point<T> top_left() const { return origin; }

    /// Top-right corner.
    constexpr Point<T> top_right() const { return Point<T>{right(), origin.y}; }

    /// Bottom-left corner.
    constexpr Point<T> bottom_left() const { return Point<T>{origin.x, bottom()}; }

    /// Bottom-right corner.
    constexpr Point<T> bottom_right() const { return Point<T>{right(), bottom()}; }

    /// Check if a point is inside the bounds.
    constexpr bool contains(const Point<T>& point) const {
        return point.x >= origin.x && point.x <= right() &&
               point.y >= origin.y && point.y <= bottom();
    }

    /// Check if two bounds intersect.
    constexpr bool intersects(const Bounds& other) const {
        return origin.x < other.right() && right() > other.origin.x &&
               origin.y < other.bottom() && bottom() > other.origin.y;
    }

    /// Compute the intersection of two bounds.
    constexpr std::optional<Bounds> intersection(const Bounds& other) const {
        T x = std::max(origin.x, other.origin.x);
        T y = std::max(origin.y, other.origin.y);
        T r = std::min(right(), other.right());
        T b = std::min(bottom(), other.bottom());
        if (r > x && b > y) {
            return Bounds{Point<T>{x, y}, Size<T>{r - x, b - y}};
        }
        return std::nullopt;
    }

    /// Compute the union (bounding box) of two bounds.
    constexpr Bounds union_with(const Bounds& other) const {
        T x = std::min(origin.x, other.origin.x);
        T y = std::min(origin.y, other.origin.y);
        T r = std::max(right(), other.right());
        T b = std::max(bottom(), other.bottom());
        return Bounds{Point<T>{x, y}, Size<T>{r - x, b - y}};
    }

    /// Inset the bounds by the given amounts on each edge.
    constexpr Bounds inset(T top, T right_val, T bottom_val, T left) const {
        return Bounds{
            Point<T>{origin.x + left, origin.y + top},
            Size<T>{size.width - left - right_val, size.height - top - bottom_val}
        };
    }

    /// Convert a point from absolute coordinates to local (relative to origin).
    constexpr Point<T> localize(const Point<T>& point) const {
        return point - origin;
    }

    /// Dilate (expand) the bounds by the given amount on all sides.
    constexpr Bounds dilate(T amount) const {
        return Bounds{
            Point<T>{origin.x - amount, origin.y - amount},
            Size<T>{size.width + amount + amount, size.height + amount + amount}
        };
    }

    static constexpr Bounds zero() { return Bounds{Point<T>::zero(), Size<T>::zero()}; }
};

// ─── Edges ─────────────────────────────────────────────────────────────────

/// Values for each edge of a rectangle (like CSS margin/padding).
template <typename T>
struct Edges {
    T top{};
    T right{};
    T bottom{};
    T left{};

    constexpr Edges() = default;
    constexpr Edges(T top, T right, T bottom, T left)
        : top(top), right(right), bottom(bottom), left(left) {}

    /// Uniform value on all edges.
    static constexpr Edges all(T value) {
        return Edges{value, value, value, value};
    }

    /// Horizontal and vertical values.
    static constexpr Edges axes(T vertical, T horizontal) {
        return Edges{vertical, horizontal, vertical, horizontal};
    }

    constexpr bool operator==(const Edges& other) const {
        return top == other.top && right == other.right &&
               bottom == other.bottom && left == other.left;
    }

    /// Sum of horizontal edges.
    constexpr T horizontal() const { return left + right; }

    /// Sum of vertical edges.
    constexpr T vertical() const { return top + bottom; }

    /// Apply a function to each edge value.
    template <typename F>
    auto map(F&& f) const {
        using R = decltype(f(top));
        return Edges<R>{f(top), f(right), f(bottom), f(left)};
    }

    static constexpr Edges zero() { return Edges{}; }
};

// ─── Corners ───────────────────────────────────────────────────────────────

/// Values for each corner of a rectangle (like CSS border-radius).
template <typename T>
struct Corners {
    T top_left{};
    T top_right{};
    T bottom_right{};
    T bottom_left{};

    constexpr Corners() = default;
    constexpr Corners(T tl, T tr, T br, T bl)
        : top_left(tl), top_right(tr), bottom_right(br), bottom_left(bl) {}

    /// Uniform value on all corners.
    static constexpr Corners all(T value) {
        return Corners{value, value, value, value};
    }

    constexpr bool operator==(const Corners& other) const {
        return top_left == other.top_left && top_right == other.top_right &&
               bottom_right == other.bottom_right && bottom_left == other.bottom_left;
    }

    /// Apply a function to each corner value.
    template <typename F>
    auto map(F&& f) const {
        using R = decltype(f(top_left));
        return Corners<R>{f(top_left), f(top_right), f(bottom_right), f(bottom_left)};
    }

    /// Clamp corner radii so they don't exceed half of the given size.
    Corners clamp_for_size(Size<T> size) const {
        T half_w = size.width / T{2};
        T half_h = size.height / T{2};
        T max_radius = std::min(half_w, half_h);
        return Corners{
            std::min(top_left, max_radius),
            std::min(top_right, max_radius),
            std::min(bottom_right, max_radius),
            std::min(bottom_left, max_radius),
        };
    }

    static constexpr Corners zero() { return Corners{}; }
};

// ─── Convenience literals ──────────────────────────────────────────────────

namespace literals {

constexpr Pixels operator""_px(long double v) { return Pixels{static_cast<float>(v)}; }
constexpr Pixels operator""_px(unsigned long long v) { return Pixels{static_cast<float>(v)}; }
constexpr Rems operator""_rem(long double v) { return Rems{static_cast<float>(v)}; }
constexpr Rems operator""_rem(unsigned long long v) { return Rems{static_cast<float>(v)}; }

} // namespace literals

// ─── Convenience type aliases ──────────────────────────────────────────────

using PixelPoint = Point<Pixels>;
using PixelSize = Size<Pixels>;
using PixelBounds = Bounds<Pixels>;

} // namespace gpui
