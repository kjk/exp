#include <gpui/color.h>
#include <algorithm>
#include <cmath>

namespace gpui {

// ─── HSLA to RGBA conversion ───────────────────────────────────────────────

static float hue_to_rgb(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
}

Rgba Hsla::to_rgba() const {
    if (s == 0.0f) {
        // Achromatic (gray)
        return Rgba{l, l, l, a};
    }

    float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    float p = 2.0f * l - q;

    float r = hue_to_rgb(p, q, h + 1.0f / 3.0f);
    float g = hue_to_rgb(p, q, h);
    float b = hue_to_rgb(p, q, h - 1.0f / 3.0f);

    return Rgba{
        std::clamp(r, 0.0f, 1.0f),
        std::clamp(g, 0.0f, 1.0f),
        std::clamp(b, 0.0f, 1.0f),
        a
    };
}

// ─── RGBA to HSLA conversion ───────────────────────────────────────────────

Hsla Hsla::from_rgba(const Rgba& rgba) {
    float r = rgba.r;
    float g = rgba.g;
    float b = rgba.b;

    float max_c = std::max({r, g, b});
    float min_c = std::min({r, g, b});
    float delta = max_c - min_c;

    float l = (max_c + min_c) / 2.0f;

    if (delta == 0.0f) {
        // Achromatic
        return Hsla{0.0f, 0.0f, l, rgba.a};
    }

    float s = l > 0.5f
        ? delta / (2.0f - max_c - min_c)
        : delta / (max_c + min_c);

    float h = 0.0f;
    if (max_c == r) {
        h = std::fmod((g - b) / delta, 6.0f);
    } else if (max_c == g) {
        h = (b - r) / delta + 2.0f;
    } else {
        h = (r - g) / delta + 4.0f;
    }

    h /= 6.0f;
    if (h < 0.0f) h += 1.0f;

    return Hsla{h, s, l, rgba.a};
}

} // namespace gpui
