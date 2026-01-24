#pragma once

/// C++ port of GPUI's scene module.
/// Defines rendering primitives that make up a frame to be painted.

#include <gpui/color.h>
#include <gpui/geometry.h>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

namespace gpui {

// ─── Content Mask ──────────────────────────────────────────────────────────

/// A clipping rectangle for rendering operations.
struct ContentMask {
    Bounds<Pixels> bounds;
    Corners<Pixels> corner_radii;
};

// ─── Hitbox ────────────────────────────────────────────────────────────────

/// An opaque ID for a hitbox used in hit-testing.
struct HitboxId {
    uint64_t value = 0;

    bool operator==(const HitboxId& other) const { return value == other.value; }
    bool operator!=(const HitboxId& other) const { return value != other.value; }
};

/// A region that can receive mouse events.
struct Hitbox {
    HitboxId id;
    Bounds<Pixels> bounds;
    ContentMask mask;
    bool opaque = true;
};

// ─── Quad ──────────────────────────────────────────────────────────────────

/// A filled/stroked rectangle with rounded corners.
struct Quad {
    Bounds<Pixels> bounds;
    Background background;
    Rgba border_color;
    Edges<Pixels> border_widths;
    Corners<Pixels> corner_radii;
};

// ─── Shadow ────────────────────────────────────────────────────────────────

/// A box shadow (drop shadow or inner shadow).
struct Shadow {
    Bounds<Pixels> bounds;
    Corners<Pixels> corner_radii;
    Rgba color;
    Pixels blur_radius;
    Point<Pixels> offset;
};

// ─── Path ──────────────────────────────────────────────────────────────────

/// A vertex in a path.
struct PathVertex {
    Point<Pixels> position;
    Point<Pixels> control_point;  // Bezier control point (same as position if straight)
};

/// A filled vector path.
struct Path {
    Bounds<Pixels> bounds;
    std::vector<PathVertex> vertices;
    Rgba color;
    std::optional<ContentMask> clip;
};

// ─── Underline ─────────────────────────────────────────────────────────────

/// A text underline (or strikethrough).
struct Underline {
    Point<Pixels> origin;
    Pixels width;
    Pixels thickness;
    Rgba color;
    bool wavy = false;
};

// ─── Sprites ───────────────────────────────────────────────────────────────

/// An opaque ID for a texture atlas tile.
struct AtlasTileId {
    uint32_t value = 0;
};

/// Texture atlas tile bounds within the atlas texture.
struct AtlasTile {
    AtlasTileId id;
    Bounds<DevicePixels> bounds;
};

/// A single-color (alpha-only) sprite, typically for monochrome glyphs.
struct MonochromeSprite {
    Bounds<Pixels> bounds;
    Rgba color;
    AtlasTile tile;
    // Transformation for the sprite
    struct Transform {
        float m[4] = {1, 0, 0, 1}; // 2x2 matrix
    } transform;
};

/// A subpixel-rendered sprite (for LCD text rendering).
struct SubpixelSprite {
    Bounds<Pixels> bounds;
    Rgba color;
    AtlasTile tile;
};

/// A full-color sprite (for images, emoji, etc.).
struct PolychromeSprite {
    Bounds<Pixels> bounds;
    AtlasTile tile;
    float opacity = 1.0f;
    bool grayscale = false;
};

// ─── Paint Surface ─────────────────────────────────────────────────────────

/// A platform-specific rendered surface (e.g., video frame, custom GPU content).
struct PaintSurface {
    Bounds<Pixels> bounds;
    // Platform-specific handle (opaque pointer)
    void* native_surface = nullptr;
};

// ─── Scene ─────────────────────────────────────────────────────────────────

/// The order in which a primitive should be drawn.
struct DrawOrder {
    uint32_t layer = 0;
    uint32_t index = 0;

    bool operator<(const DrawOrder& other) const {
        if (layer != other.layer) return layer < other.layer;
        return index < other.index;
    }
};

/// A single rendering primitive with its draw order.
struct Primitive {
    DrawOrder order;
    std::variant<
        Quad,
        Shadow,
        Path,
        Underline,
        MonochromeSprite,
        SubpixelSprite,
        PolychromeSprite,
        PaintSurface
    > content;
};

/// A batch of primitives of the same type, ready for GPU submission.
struct PrimitiveBatch {
    enum class Kind {
        Quads,
        Shadows,
        Paths,
        Underlines,
        MonochromeSprites,
        SubpixelSprites,
        PolychromeSprites,
        Surfaces,
    };

    Kind kind;
    std::vector<const Primitive*> primitives;
};

/// A scene is the complete set of rendering primitives for a single frame.
class Scene {
public:
    Scene() = default;

    /// Push a new layer onto the layer stack.
    void push_layer() { current_layer_++; }

    /// Pop the current layer.
    void pop_layer() {
        if (current_layer_ > 0) current_layer_--;
    }

    /// Insert a quad into the scene.
    void insert_quad(Quad quad) {
        Primitive p;
        p.order = next_order();
        p.content = std::move(quad);
        primitives_.push_back(std::move(p));
    }

    /// Insert a shadow into the scene.
    void insert_shadow(Shadow shadow) {
        Primitive p;
        p.order = next_order();
        p.content = std::move(shadow);
        primitives_.push_back(std::move(p));
    }

    /// Insert a path into the scene.
    void insert_path(Path path) {
        Primitive p;
        p.order = next_order();
        p.content = std::move(path);
        primitives_.push_back(std::move(p));
    }

    /// Insert an underline into the scene.
    void insert_underline(Underline underline) {
        Primitive p;
        p.order = next_order();
        p.content = std::move(underline);
        primitives_.push_back(std::move(p));
    }

    /// Insert a monochrome sprite.
    void insert_monochrome_sprite(MonochromeSprite sprite) {
        Primitive p;
        p.order = next_order();
        p.content = std::move(sprite);
        primitives_.push_back(std::move(p));
    }

    /// Insert a subpixel sprite.
    void insert_subpixel_sprite(SubpixelSprite sprite) {
        Primitive p;
        p.order = next_order();
        p.content = std::move(sprite);
        primitives_.push_back(std::move(p));
    }

    /// Insert a polychrome sprite.
    void insert_polychrome_sprite(PolychromeSprite sprite) {
        Primitive p;
        p.order = next_order();
        p.content = std::move(sprite);
        primitives_.push_back(std::move(p));
    }

    /// Insert a surface.
    void insert_surface(PaintSurface surface) {
        Primitive p;
        p.order = next_order();
        p.content = std::move(surface);
        primitives_.push_back(std::move(p));
    }

    /// Sort primitives by draw order (call before rendering).
    void finish() {
        std::sort(primitives_.begin(), primitives_.end(),
                  [](const Primitive& a, const Primitive& b) {
                      return a.order < b.order;
                  });
    }

    /// Get all primitives in draw order.
    const std::vector<Primitive>& primitives() const { return primitives_; }

    /// Group primitives into batches by type for efficient rendering.
    std::vector<PrimitiveBatch> batches() const;

    /// Clear all primitives.
    void clear() {
        primitives_.clear();
        current_layer_ = 0;
        next_index_ = 0;
    }

private:
    DrawOrder next_order() {
        return DrawOrder{current_layer_, next_index_++};
    }

    std::vector<Primitive> primitives_;
    uint32_t current_layer_ = 0;
    uint32_t next_index_ = 0;
};

} // namespace gpui
