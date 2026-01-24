#include <gpui/elements/text.h>
#include <gpui/window.h>
#include <gpui/app.h>

namespace gpui {

// ─── TextElement ───────────────────────────────────────────────────────────

LayoutId TextElement::request_layout(
    const std::optional<GlobalElementId>&,
    Window& window, App&) {

    Style resolved;
    resolved.apply(style_);

    // Build the text content
    std::string full_text;
    std::vector<TextRun> text_runs;

    Pixels font_size = resolved.text.font_size
        ? resolved.text.font_size->to_pixels(window.rem_size())
        : window.rem_size();

    for (const auto& run : runs_) {
        TextRun tr;
        tr.length = run.text.size();
        tr.font.family = run.style.font_family.value_or(
            resolved.text.font_family.value_or("sans-serif"));
        tr.font.weight = run.style.font_weight.value_or(
            resolved.text.font_weight.value_or(FontWeight::normal()));
        tr.font.style = run.style.font_style.value_or(
            resolved.text.font_style.value_or(FontStyle::Normal));
        tr.font_size = run.style.font_size
            ? run.style.font_size->to_pixels(window.rem_size())
            : font_size;
        tr.color = run.style.color.value_or(
            resolved.text.color.value_or(white()));
        tr.background_color = run.style.background_color;
        tr.decoration = run.style.decoration;
        text_runs.push_back(std::move(tr));
        full_text += run.text;
    }

    // Use a measured layout node so text can wrap
    auto measure_fn = [this, full_text = std::move(full_text),
                       text_runs = std::move(text_runs), font_size]
        (Size<std::optional<Pixels>> available) -> Size<Pixels> {

        TextLayoutOptions options;
        options.max_width = available.width;
        if (max_lines_) options.max_lines = *max_lines_;

        // Use a simple measurement heuristic
        Pixels advance{font_size.value * 0.6f};
        Pixels total_width{0.0f};
        Pixels max_line_width = available.width.value_or(Pixels{99999.0f});
        Pixels current_width{0.0f};
        int line_count = 1;

        for (char c : full_text) {
            if (c == '\n') {
                if (current_width > total_width) total_width = current_width;
                current_width = Pixels{0.0f};
                line_count++;
            } else {
                current_width += advance;
                if (current_width > max_line_width) {
                    if (current_width - advance > total_width)
                        total_width = Pixels{current_width.value - advance.value};
                    current_width = advance;
                    line_count++;
                }
            }
        }
        if (current_width > total_width) total_width = current_width;

        if (max_lines_ && line_count > static_cast<int>(*max_lines_)) {
            line_count = static_cast<int>(*max_lines_);
        }

        Pixels line_h{font_size.value * 1.2f};
        return Size<Pixels>{
            std::min(total_width, max_line_width),
            Pixels{line_h.value * line_count}
        };
    };

    Style layout_style;
    layout_style.apply(style_);
    return window.request_measured_layout(layout_style, std::move(measure_fn));
}

void TextElement::prepaint(
    const std::optional<GlobalElementId>&,
    Bounds<Pixels> bounds, Window& window, App&) {

    // Build layout for the text within the given bounds
    std::string full_text;
    std::vector<TextRun> text_runs;

    Style resolved;
    resolved.apply(style_);

    Pixels font_size = resolved.text.font_size
        ? resolved.text.font_size->to_pixels(window.rem_size())
        : window.rem_size();

    for (const auto& run : runs_) {
        TextRun tr;
        tr.length = run.text.size();
        tr.font.family = run.style.font_family.value_or("sans-serif");
        tr.font.weight = run.style.font_weight.value_or(FontWeight::normal());
        tr.font.style = run.style.font_style.value_or(FontStyle::Normal);
        tr.font_size = run.style.font_size
            ? run.style.font_size->to_pixels(window.rem_size())
            : font_size;
        tr.color = run.style.color.value_or(white());
        text_runs.push_back(std::move(tr));
        full_text += run.text;
    }

    TextLayoutOptions options;
    options.max_width = bounds.size.width;
    if (max_lines_) options.max_lines = *max_lines_;

    cached_layout_ = window.text_system().layout_text(full_text, text_runs, options);
}

void TextElement::paint(
    const std::optional<GlobalElementId>&,
    Bounds<Pixels> bounds, Window& window, App&) {

    if (!cached_layout_) return;

    Style resolved;
    resolved.apply(style_);

    Rgba text_color = resolved.text.color.value_or(white());

    // Paint each glyph as a monochrome sprite
    Pixels y_offset = bounds.origin.y;
    for (const auto& line : cached_layout_->lines) {
        for (const auto& glyph : line.glyphs) {
            MonochromeSprite sprite;
            sprite.bounds = Bounds<Pixels>{
                Point<Pixels>{
                    bounds.origin.x + glyph.position.x,
                    y_offset + glyph.position.y
                },
                glyph.size
            };
            sprite.color = text_color;
            sprite.tile = AtlasTile{AtlasTileId{glyph.id.value}, {}};
            window.paint_glyph(std::move(sprite));
        }
        y_offset += line.height();
    }

    // Paint decorations
    if (resolved.text.decoration) {
        const auto& dec = *resolved.text.decoration;
        if (dec.line != TextDecoration::Line::None) {
            Rgba dec_color = dec.color.value_or(text_color);
            Pixels thickness{1.0f};
            Pixels y_pos = bounds.origin.y;

            for (const auto& line : cached_layout_->lines) {
                Pixels underline_y = y_pos;
                if (dec.line == TextDecoration::Line::Underline) {
                    underline_y += line.ascent;
                } else {
                    underline_y += Pixels{line.ascent.value * 0.5f};
                }

                Underline ul;
                ul.origin = Point<Pixels>{bounds.origin.x, underline_y};
                ul.width = line.width;
                ul.thickness = thickness;
                ul.color = dec_color;
                ul.wavy = (dec.style == TextDecoration::Style::Wavy);
                window.paint_underline(std::move(ul));

                y_pos += line.height();
            }
        }
    }
}

// ─── StringElement ─────────────────────────────────────────────────────────

LayoutId StringElement::request_layout(
    const std::optional<GlobalElementId>& global_id,
    Window& window, App& cx) {
    inner_ = TextElement(text_);
    return inner_.request_layout(global_id, window, cx);
}

void StringElement::prepaint(
    const std::optional<GlobalElementId>& global_id,
    Bounds<Pixels> bounds, Window& window, App& cx) {
    inner_.prepaint(global_id, bounds, window, cx);
}

void StringElement::paint(
    const std::optional<GlobalElementId>& global_id,
    Bounds<Pixels> bounds, Window& window, App& cx) {
    inner_.paint(global_id, bounds, window, cx);
}

} // namespace gpui
