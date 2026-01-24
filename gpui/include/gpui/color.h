#pragma once

/// C++ port of GPUI's color module.
/// Provides RGBA/HSLA color types with conversions, blending, and gradient support.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace gpui {

// ─── Rgba ──────────────────────────────────────────────────────────────────

/// A color in red-green-blue-alpha format. Components are in [0.0, 1.0].
struct Rgba {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    constexpr Rgba() = default;
    constexpr Rgba(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    /// Create from a 24-bit hex value (0xRRGGBB).
    static constexpr Rgba from_rgb(uint32_t hex) {
        return Rgba{
            static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
            static_cast<float>((hex >> 8) & 0xFF) / 255.0f,
            static_cast<float>(hex & 0xFF) / 255.0f,
            1.0f
        };
    }

    /// Create from a 32-bit hex value (0xRRGGBBAA).
    static constexpr Rgba from_rgba(uint32_t hex) {
        return Rgba{
            static_cast<float>((hex >> 24) & 0xFF) / 255.0f,
            static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
            static_cast<float>((hex >> 8) & 0xFF) / 255.0f,
            static_cast<float>(hex & 0xFF) / 255.0f
        };
    }

    /// Blend this color over another (alpha compositing, source-over).
    constexpr Rgba blend(const Rgba& background) const {
        if (a >= 1.0f) return *this;
        if (a <= 0.0f) return background;

        float out_a = a + background.a * (1.0f - a);
        if (out_a == 0.0f) return Rgba{0, 0, 0, 0};

        return Rgba{
            (r * a + background.r * background.a * (1.0f - a)) / out_a,
            (g * a + background.g * background.a * (1.0f - a)) / out_a,
            (b * a + background.b * background.a * (1.0f - a)) / out_a,
            out_a
        };
    }

    /// Multiply the alpha by a factor.
    constexpr Rgba with_alpha(float alpha) const {
        return Rgba{r, g, b, a * alpha};
    }

    constexpr bool operator==(const Rgba& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    /// Convert to a 32-bit packed RGBA value.
    constexpr uint32_t to_u32() const {
        auto to_byte = [](float v) -> uint32_t {
            return static_cast<uint32_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
        };
        return (to_byte(r) << 24) | (to_byte(g) << 16) | (to_byte(b) << 8) | to_byte(a);
    }
};

// ─── Hsla ──────────────────────────────────────────────────────────────────

/// A color in hue-saturation-lightness-alpha format.
/// Hue is in [0.0, 1.0] (representing 0-360 degrees), S/L/A in [0.0, 1.0].
struct Hsla {
    float h = 0.0f;
    float s = 0.0f;
    float l = 0.0f;
    float a = 1.0f;

    constexpr Hsla() = default;
    constexpr Hsla(float h, float s, float l, float a = 1.0f) : h(h), s(s), l(l), a(a) {}

    /// Convert to RGBA.
    Rgba to_rgba() const;

    /// Create from RGBA.
    static Hsla from_rgba(const Rgba& rgba);

    /// Make this color grayscale (zero saturation).
    constexpr Hsla grayscale() const { return Hsla{h, 0.0f, l, a}; }

    /// Reduce opacity by the given amount.
    constexpr Hsla fade_out(float amount) const {
        return Hsla{h, s, l, std::max(0.0f, a - amount)};
    }

    /// Set absolute opacity.
    constexpr Hsla with_opacity(float opacity) const {
        return Hsla{h, s, l, opacity};
    }

    /// Set the alpha channel.
    constexpr Hsla with_alpha(float alpha) const {
        return Hsla{h, s, l, alpha};
    }

    constexpr bool operator==(const Hsla& other) const {
        return h == other.h && s == other.s && l == other.l && a == other.a;
    }
};

// ─── Color convenience functions ───────────────────────────────────────────

/// Create an RGBA color from a hex value (e.g., 0xFF0000 for red).
constexpr Rgba rgb(uint32_t hex) { return Rgba::from_rgb(hex); }

/// Create an RGBA color from a hex value with alpha.
constexpr Rgba rgba(uint32_t hex) { return Rgba::from_rgba(hex); }

/// Create an HSLA color from individual components.
constexpr Hsla hsla(float h, float s, float l, float a = 1.0f) {
    return Hsla{h, s, l, a};
}

inline Rgba black() { return Rgba{0.0f, 0.0f, 0.0f, 1.0f}; }
inline Rgba white() { return Rgba{1.0f, 1.0f, 1.0f, 1.0f}; }
inline Rgba red() { return Rgba{1.0f, 0.0f, 0.0f, 1.0f}; }
inline Rgba green() { return Rgba{0.0f, 1.0f, 0.0f, 1.0f}; }
inline Rgba blue() { return Rgba{0.0f, 0.0f, 1.0f, 1.0f}; }
inline Rgba yellow() { return Rgba{1.0f, 1.0f, 0.0f, 1.0f}; }
inline Rgba transparent() { return Rgba{0.0f, 0.0f, 0.0f, 0.0f}; }

// ─── Color Space ───────────────────────────────────────────────────────────

/// Color interpolation space for gradients.
enum class ColorSpace {
    Srgb,
    Oklab,
};

// ─── Gradient & Background ─────────────────────────────────────────────────

/// A color stop in a gradient, with a position in [0.0, 1.0].
struct LinearColorStop {
    Rgba color;
    float position = 0.0f;
};

/// A linear gradient with an angle and color stops.
struct LinearGradient {
    float angle_degrees = 0.0f;
    ColorSpace color_space = ColorSpace::Srgb;
    std::vector<LinearColorStop> stops;
};

/// Background fill: solid color, gradient, or pattern.
struct Background {
    enum class Tag { Solid, LinearGradient, PatternSlash };

    Tag tag = Tag::Solid;
    Rgba solid_color;
    gpui::LinearGradient gradient;

    Background() = default;

    static Background solid(Rgba color) {
        Background bg;
        bg.tag = Tag::Solid;
        bg.solid_color = color;
        return bg;
    }

    static Background linear_gradient(gpui::LinearGradient grad) {
        Background bg;
        bg.tag = Tag::LinearGradient;
        bg.gradient = std::move(grad);
        return bg;
    }

    static Background pattern_slash(Rgba color) {
        Background bg;
        bg.tag = Tag::PatternSlash;
        bg.solid_color = color;
        return bg;
    }
};

/// Create a solid background from a color.
inline Background solid_background(Rgba color) { return Background::solid(color); }

/// Create a linear gradient background.
inline Background linear_gradient(float angle, std::vector<LinearColorStop> stops,
                                   ColorSpace space = ColorSpace::Srgb) {
    LinearGradient grad;
    grad.angle_degrees = angle;
    grad.color_space = space;
    grad.stops = std::move(stops);
    return Background::linear_gradient(std::move(grad));
}

/// Create a color stop for a gradient.
inline LinearColorStop linear_color_stop(Rgba color, float position) {
    return LinearColorStop{color, position};
}

} // namespace gpui
