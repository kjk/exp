#include <gpui/text_system.h>
#include <algorithm>
#include <numeric>

namespace gpui {

std::optional<std::pair<std::size_t, std::size_t>> TextLayout::hit_test(Point<Pixels> point) const {
    if (lines.empty()) return std::nullopt;

    // Find the line that contains the y position.
    Pixels y{0.0f};
    for (std::size_t line_idx = 0; line_idx < lines.size(); ++line_idx) {
        const auto& line = lines[line_idx];
        Pixels line_h = line.height();
        if (point.y <= y + line_h || line_idx == lines.size() - 1) {
            // Found the line. Now find the glyph.
            if (line.glyphs.empty()) {
                return std::make_pair(line_idx, std::size_t{0});
            }

            for (std::size_t glyph_idx = 0; glyph_idx < line.glyphs.size(); ++glyph_idx) {
                const auto& glyph = line.glyphs[glyph_idx];
                Pixels glyph_center{glyph.position.x.value + glyph.size.width.value * 0.5f};
                if (point.x <= glyph_center) {
                    return std::make_pair(line_idx, glyph.byte_index);
                }
            }

            // Past the end of the line
            return std::make_pair(line_idx, line.glyphs.back().byte_index + 1);
        }
        y += line_h;
    }

    return std::nullopt;
}

// ─── Stub text system for platforms without a real implementation ───────────

class StubTextSystem : public TextSystem {
public:
    FontId load_font(const Font&) override { return FontId{next_id_++}; }

    Pixels ascent(FontId, Pixels size) override { return Pixels{size.value * 0.8f}; }
    Pixels descent(FontId, Pixels size) override { return Pixels{size.value * 0.2f}; }
    Pixels line_height(FontId, Pixels size) override { return size; }

    Pixels glyph_advance(FontId, Pixels size, GlyphId) override {
        return Pixels{size.value * 0.6f};  // Approximate monospace advance
    }

    ShapedLine shape_line(const std::string& text_str,
                          const std::vector<TextRun>& runs,
                          Pixels font_size) override {
        ShapedLine line;
        Pixels x{0.0f};
        Pixels advance{font_size.value * 0.6f};

        for (std::size_t i = 0; i < text_str.size(); ++i) {
            if (text_str[i] == '\n') break;

            ShapedGlyph glyph;
            glyph.id = GlyphId{static_cast<uint32_t>(text_str[i])};
            glyph.font_id = FontId{1};
            glyph.position = Point<Pixels>{x, Pixels{0}};
            glyph.size = Size<Pixels>{advance, font_size};
            glyph.byte_index = i;
            line.glyphs.push_back(glyph);
            x += advance;
        }

        line.width = x;
        line.ascent = Pixels{font_size.value * 0.8f};
        line.descent = Pixels{font_size.value * 0.2f};
        return line;
    }

    TextLayout layout_text(const std::string& text_str,
                           const std::vector<TextRun>& runs,
                           const TextLayoutOptions& options) override {
        TextLayout layout;
        Pixels font_size{16.0f};
        if (!runs.empty()) font_size = runs[0].font_size;

        // Simple line-breaking implementation
        std::string current_line;
        Pixels x{0.0f};
        Pixels advance{font_size.value * 0.6f};
        Pixels max_width = options.max_width.value_or(Pixels{std::numeric_limits<float>::max()});
        Pixels total_height{0.0f};
        Pixels max_line_width{0.0f};

        for (std::size_t i = 0; i < text_str.size(); ++i) {
            char c = text_str[i];
            if (c == '\n' || (x + advance > max_width && !current_line.empty())) {
                auto line = shape_line(current_line, runs, font_size);
                Pixels line_h = line.height();
                layout.line_bounds.push_back(Bounds<Pixels>{
                    Point<Pixels>{Pixels{0}, total_height},
                    Size<Pixels>{line.width, line_h}
                });
                if (line.width > max_line_width) max_line_width = line.width;
                total_height += Pixels{line_h.value * options.line_height_multiplier};
                layout.lines.push_back(std::move(line));
                current_line.clear();
                x = Pixels{0.0f};

                if (options.max_lines && layout.lines.size() >= *options.max_lines) break;
                if (c == '\n') continue;
            }
            current_line += c;
            x += advance;
        }

        if (!current_line.empty() &&
            (!options.max_lines || layout.lines.size() < *options.max_lines)) {
            auto line = shape_line(current_line, runs, font_size);
            Pixels line_h = line.height();
            layout.line_bounds.push_back(Bounds<Pixels>{
                Point<Pixels>{Pixels{0}, total_height},
                Size<Pixels>{line.width, line_h}
            });
            if (line.width > max_line_width) max_line_width = line.width;
            total_height += line_h;
            layout.lines.push_back(std::move(line));
        }

        layout.size = Size<Pixels>{max_line_width, total_height};
        return layout;
    }

    RasterizedGlyph rasterize_glyph(FontId, Pixels size, GlyphId, float) override {
        // Return an empty glyph bitmap (stub)
        int w = static_cast<int>(size.value * 0.6f);
        int h = static_cast<int>(size.value);
        return RasterizedGlyph{
            Size<DevicePixels>{DevicePixels{w}, DevicePixels{h}},
            Point<DevicePixels>{DevicePixels{0}, DevicePixels{0}},
            std::vector<uint8_t>(w * h, 0),
            false
        };
    }

    std::vector<FontFamily> available_families() override {
        return {"sans-serif", "serif", "monospace"};
    }

    Font resolve_font(const Font& request) override {
        return request;
    }

private:
    uint32_t next_id_ = 1;
};

std::unique_ptr<TextSystem> create_text_system() {
    // TODO: Use platform-specific implementation (CoreText, DirectWrite, HarfBuzz)
    return std::make_unique<StubTextSystem>();
}

} // namespace gpui
