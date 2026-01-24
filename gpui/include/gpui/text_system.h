#pragma once

/// C++ port of GPUI's text system.
/// Provides text layout, shaping, and rendering abstractions.

#include <gpui/color.h>
#include <gpui/geometry.h>
#include <gpui/style.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace gpui {

// ─── Font Types ────────────────────────────────────────────────────────────

/// A font family name.
using FontFamily = std::string;

/// A specific font identified by family, weight, and style.
struct Font {
    FontFamily family;
    FontWeight weight = FontWeight::normal();
    FontStyle style = FontStyle::Normal;

    bool operator==(const Font& other) const {
        return family == other.family && weight == other.weight && style == other.style;
    }
};

/// An opaque handle to a loaded font face.
struct FontId {
    uint32_t value = 0;
    bool operator==(const FontId& other) const { return value == other.value; }
};

/// A glyph identifier within a font.
struct GlyphId {
    uint32_t value = 0;
};

// ─── Text Runs ─────────────────────────────────────────────────────────────

/// A run of text with uniform styling.
struct TextRun {
    std::size_t length = 0;  // Number of UTF-8 bytes in this run
    Font font;
    Pixels font_size;
    Rgba color;
    std::optional<Rgba> background_color;
    std::optional<TextDecoration> decoration;
};

// ─── Shaped Text ───────────────────────────────────────────────────────────

/// A shaped glyph ready for rendering.
struct ShapedGlyph {
    GlyphId id;
    FontId font_id;
    Point<Pixels> position;    // Position relative to the line origin
    Size<Pixels> size;         // Bounding box of the glyph
    std::size_t byte_index;    // Index into the source text
};

/// A line of shaped text.
struct ShapedLine {
    std::vector<ShapedGlyph> glyphs;
    Pixels width;
    Pixels ascent;
    Pixels descent;

    Pixels height() const { return Pixels{ascent.value + descent.value}; }
};

/// The result of laying out a block of text.
struct TextLayout {
    std::vector<ShapedLine> lines;
    Size<Pixels> size;
    std::vector<Bounds<Pixels>> line_bounds;  // Bounds for each line

    /// Get the line index and byte offset for a given point.
    std::optional<std::pair<std::size_t, std::size_t>> hit_test(Point<Pixels> point) const;
};

// ─── Line Layout ───────────────────────────────────────────────────────────

/// Options for text layout.
struct TextLayoutOptions {
    std::optional<Pixels> max_width;       // Wrap width
    std::optional<std::size_t> max_lines;  // Maximum number of lines
    TextOverflow overflow = TextOverflow::Wrap;
    float line_height_multiplier = 1.2f;
};

// ─── Text System ───────────────────────────────────────────────────────────

/// Abstract interface for text shaping and font management.
/// Platform implementations use Core Text (macOS), DirectWrite (Windows),
/// or FreeType/HarfBuzz (Linux).
class TextSystem {
public:
    virtual ~TextSystem() = default;

    /// Load a font and return its ID.
    virtual FontId load_font(const Font& font) = 0;

    /// Get the metrics for a font at a given size.
    virtual Pixels ascent(FontId font, Pixels size) = 0;
    virtual Pixels descent(FontId font, Pixels size) = 0;
    virtual Pixels line_height(FontId font, Pixels size) = 0;

    /// Measure the advance width of a glyph.
    virtual Pixels glyph_advance(FontId font, Pixels size, GlyphId glyph) = 0;

    /// Shape a text string into glyphs.
    virtual ShapedLine shape_line(
        const std::string& text,
        const std::vector<TextRun>& runs,
        Pixels font_size) = 0;

    /// Perform full text layout with wrapping.
    virtual TextLayout layout_text(
        const std::string& text,
        const std::vector<TextRun>& runs,
        const TextLayoutOptions& options) = 0;

    /// Rasterize a glyph to a bitmap for the sprite atlas.
    struct RasterizedGlyph {
        Size<DevicePixels> size;
        Point<DevicePixels> offset;
        std::vector<uint8_t> bitmap;  // Alpha values (monochrome) or RGBA
        bool is_color = false;        // True for color emoji
    };

    virtual RasterizedGlyph rasterize_glyph(
        FontId font, Pixels size, GlyphId glyph, float scale_factor) = 0;

    /// Get available font families on the system.
    virtual std::vector<FontFamily> available_families() = 0;

    /// Resolve a font family name, falling back to a default if not found.
    virtual Font resolve_font(const Font& request) = 0;
};

/// Create the platform-appropriate text system.
std::unique_ptr<TextSystem> create_text_system();

} // namespace gpui
