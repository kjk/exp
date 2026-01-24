#pragma once

/// C++ port of GPUI's style module.
/// Provides a comprehensive CSS-like styling system for UI elements.

#include <gpui/color.h>
#include <gpui/geometry.h>
#include <gpui/input.h>
#include <gpui/refineable.h>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace gpui {

// ─── Display ───────────────────────────────────────────────────────────────

/// How the element participates in layout.
enum class Display {
    Block,
    Flex,
    Grid,
    None,
};

/// Element visibility.
enum class Visibility {
    Visible,
    Hidden,
};

/// Positioning scheme.
enum class Position {
    Relative,
    Absolute,
};

/// Overflow handling.
enum class Overflow {
    Visible,
    Clip,
    Hidden,
    Scroll,
};

// ─── Flex Layout ───────────────────────────────────────────────────────────

/// Flex container direction.
enum class FlexDirection {
    Row,
    RowReverse,
    Column,
    ColumnReverse,
};

/// Flex wrap behavior.
enum class FlexWrap {
    NoWrap,
    Wrap,
    WrapReverse,
};

// ─── Alignment ─────────────────────────────────────────────────────────────

/// Alignment along the cross axis.
enum class AlignItems {
    Start,
    End,
    Center,
    Baseline,
    Stretch,
};

/// Alignment for a single item (overrides AlignItems).
enum class AlignSelf {
    Auto,
    Start,
    End,
    Center,
    Baseline,
    Stretch,
};

/// Alignment of content lines in a multi-line flex container.
enum class AlignContent {
    Start,
    End,
    Center,
    Stretch,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
};

/// Alignment along the main axis.
enum class JustifyContent {
    Start,
    End,
    Center,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
};

// ─── Border Style ──────────────────────────────────────────────────────────

enum class BorderStyle {
    None,
    Solid,
    Dashed,
};

// ─── Text Overflow ─────────────────────────────────────────────────────────

enum class TextOverflow {
    Wrap,
    Ellipsis,
    EllipsisStart,
};

// ─── Font Weight ───────────────────────────────────────────────────────────

/// Font weight (maps to CSS font-weight values).
struct FontWeight {
    uint16_t value = 400;

    static constexpr FontWeight thin() { return {100}; }
    static constexpr FontWeight extra_light() { return {200}; }
    static constexpr FontWeight light() { return {300}; }
    static constexpr FontWeight normal() { return {400}; }
    static constexpr FontWeight medium() { return {500}; }
    static constexpr FontWeight semi_bold() { return {600}; }
    static constexpr FontWeight bold() { return {700}; }
    static constexpr FontWeight extra_bold() { return {800}; }
    static constexpr FontWeight black() { return {900}; }

    bool operator==(const FontWeight& other) const { return value == other.value; }
};

// ─── Font Style ────────────────────────────────────────────────────────────

enum class FontStyle {
    Normal,
    Italic,
    Oblique,
};

// ─── Text Decoration ───────────────────────────────────────────────────────

struct TextDecoration {
    enum class Line { None, Underline, Strikethrough };
    enum class Style { Solid, Wavy };

    Line line = Line::None;
    Style style = Style::Solid;
    std::optional<Rgba> color;
};

// ─── Text Style ────────────────────────────────────────────────────────────

/// Comprehensive text styling properties.
struct TextStyle {
    std::optional<std::string> font_family;
    std::optional<AbsoluteLength> font_size;
    std::optional<FontWeight> font_weight;
    std::optional<FontStyle> font_style;
    std::optional<Rgba> color;
    std::optional<Rgba> background_color;
    std::optional<TextDecoration> decoration;
    std::optional<float> line_height;  // Multiplier of font size
    std::optional<AbsoluteLength> letter_spacing;
    std::optional<TextOverflow> overflow;

    void refine(const TextStyle& other) {
        refine_field(font_family, other.font_family);
        refine_field(font_size, other.font_size);
        refine_field(font_weight, other.font_weight);
        refine_field(font_style, other.font_style);
        refine_field(color, other.color);
        refine_field(background_color, other.background_color);
        refine_field(decoration, other.decoration);
        refine_field(line_height, other.line_height);
        refine_field(letter_spacing, other.letter_spacing);
        refine_field(overflow, other.overflow);
    }
};

// ─── Box Shadow ────────────────────────────────────────────────────────────

/// A CSS-like box shadow.
struct BoxShadow {
    Rgba color;
    Point<Pixels> offset;
    Pixels blur_radius;
    Pixels spread_radius;
    bool inset = false;
};

// ─── Grid Layout ───────────────────────────────────────────────────────────

/// A track definition for CSS Grid (columns/rows).
struct GridTrack {
    enum class Tag { Fixed, Fraction, Auto };

    Tag tag = Tag::Auto;
    union {
        AbsoluteLength fixed;
        float fraction;
    };

    GridTrack() : tag(Tag::Auto), fraction(0) {}

    static GridTrack from_fixed(AbsoluteLength len) {
        GridTrack t;
        t.tag = Tag::Fixed;
        t.fixed = len;
        return t;
    }

    static GridTrack from_fraction(float fr) {
        GridTrack t;
        t.tag = Tag::Fraction;
        t.fraction = fr;
        return t;
    }

    static GridTrack auto_() { return GridTrack{}; }
};

/// Grid item placement.
struct GridPlacement {
    std::optional<int> start;
    std::optional<int> span;

    static GridPlacement auto_() { return {}; }
    static GridPlacement at(int line) { return {line, std::nullopt}; }
    static GridPlacement span_n(int n) { return {std::nullopt, n}; }
};

/// Grid location (column + row placement).
struct GridLocation {
    GridPlacement col;
    GridPlacement row;
};

// ─── Style Refinement ──────────────────────────────────────────────────────

/// A refineable style - all fields are optional, allowing partial overrides.
/// This is the C++ equivalent of Rust's `StyleRefinement`.
struct StyleRefinement {
    // Display & positioning
    std::optional<Display> display;
    std::optional<Visibility> visibility;
    std::optional<Position> position;
    std::optional<Overflow> overflow_x;
    std::optional<Overflow> overflow_y;

    // Sizing
    std::optional<Length> width;
    std::optional<Length> height;
    std::optional<Length> min_width;
    std::optional<Length> min_height;
    std::optional<Length> max_width;
    std::optional<Length> max_height;
    std::optional<float> aspect_ratio;

    // Spacing
    std::optional<Edges<Length>> margin;
    std::optional<Edges<DefiniteLength>> padding;
    std::optional<Edges<DefiniteLength>> inset;

    // Flex layout
    std::optional<FlexDirection> flex_direction;
    std::optional<FlexWrap> flex_wrap;
    std::optional<float> flex_grow;
    std::optional<float> flex_shrink;
    std::optional<Length> flex_basis;
    std::optional<Size<DefiniteLength>> gap;

    // Alignment
    std::optional<AlignItems> align_items;
    std::optional<AlignSelf> align_self;
    std::optional<AlignContent> align_content;
    std::optional<JustifyContent> justify_content;

    // Border
    std::optional<Edges<Pixels>> border_widths;
    std::optional<Rgba> border_color;
    std::optional<BorderStyle> border_style;
    std::optional<Corners<Pixels>> corner_radii;

    // Visual
    std::optional<Background> background;
    std::optional<float> opacity;
    std::optional<std::vector<BoxShadow>> box_shadow;
    std::optional<CursorStyle> mouse_cursor;

    // Text
    std::optional<TextStyle> text;

    // Grid
    std::optional<std::vector<GridTrack>> grid_cols;
    std::optional<std::vector<GridTrack>> grid_rows;
    std::optional<GridLocation> grid_location;

    /// Apply refinements from another StyleRefinement (cascade).
    void refine(const StyleRefinement& other) {
        refine_field(display, other.display);
        refine_field(visibility, other.visibility);
        refine_field(position, other.position);
        refine_field(overflow_x, other.overflow_x);
        refine_field(overflow_y, other.overflow_y);
        refine_field(width, other.width);
        refine_field(height, other.height);
        refine_field(min_width, other.min_width);
        refine_field(min_height, other.min_height);
        refine_field(max_width, other.max_width);
        refine_field(max_height, other.max_height);
        refine_field(aspect_ratio, other.aspect_ratio);
        refine_field(margin, other.margin);
        refine_field(padding, other.padding);
        refine_field(inset, other.inset);
        refine_field(flex_direction, other.flex_direction);
        refine_field(flex_wrap, other.flex_wrap);
        refine_field(flex_grow, other.flex_grow);
        refine_field(flex_shrink, other.flex_shrink);
        refine_field(flex_basis, other.flex_basis);
        refine_field(gap, other.gap);
        refine_field(align_items, other.align_items);
        refine_field(align_self, other.align_self);
        refine_field(align_content, other.align_content);
        refine_field(justify_content, other.justify_content);
        refine_field(border_widths, other.border_widths);
        refine_field(border_color, other.border_color);
        refine_field(border_style, other.border_style);
        refine_field(corner_radii, other.corner_radii);
        refine_field(background, other.background);
        refine_field(opacity, other.opacity);
        refine_field(box_shadow, other.box_shadow);
        refine_field(mouse_cursor, other.mouse_cursor);
        if (other.text.has_value()) {
            if (!text.has_value()) text = TextStyle{};
            text->refine(*other.text);
        }
        refine_field(grid_cols, other.grid_cols);
        refine_field(grid_rows, other.grid_rows);
        refine_field(grid_location, other.grid_location);
    }
};

// ─── Resolved Style ────────────────────────────────────────────────────────

/// A fully resolved style with all defaults applied.
struct Style {
    Display display = Display::Block;
    Visibility visibility = Visibility::Visible;
    Position position = Position::Relative;
    Overflow overflow_x = Overflow::Visible;
    Overflow overflow_y = Overflow::Visible;

    Length width = Length::auto_();
    Length height = Length::auto_();
    Length min_width = Length::auto_();
    Length min_height = Length::auto_();
    Length max_width = Length::auto_();
    Length max_height = Length::auto_();
    std::optional<float> aspect_ratio;

    Edges<Length> margin;
    Edges<DefiniteLength> padding;
    Edges<DefiniteLength> inset;

    FlexDirection flex_direction = FlexDirection::Row;
    FlexWrap flex_wrap = FlexWrap::NoWrap;
    float flex_grow = 0.0f;
    float flex_shrink = 1.0f;
    Length flex_basis = Length::auto_();
    Size<DefiniteLength> gap;

    AlignItems align_items = AlignItems::Stretch;
    AlignSelf align_self = AlignSelf::Auto;
    AlignContent align_content = AlignContent::Start;
    JustifyContent justify_content = JustifyContent::Start;

    Edges<Pixels> border_widths;
    Rgba border_color;
    BorderStyle border_style = BorderStyle::None;
    Corners<Pixels> corner_radii;

    std::optional<Background> background;
    float opacity = 1.0f;
    std::vector<BoxShadow> box_shadow;
    CursorStyle mouse_cursor = CursorStyle::Arrow;

    TextStyle text;

    std::vector<GridTrack> grid_cols;
    std::vector<GridTrack> grid_rows;
    std::optional<GridLocation> grid_location;

    /// Apply a refinement on top of this resolved style.
    void apply(const StyleRefinement& refinement);
};

} // namespace gpui
