#pragma once

/// C++ port of GPUI's styled module.
/// Provides a fluent builder API for styling elements (similar to Tailwind CSS utility classes).

#include <gpui/color.h>
#include <gpui/geometry.h>
#include <gpui/style.h>

namespace gpui {

/// CRTP base class that provides fluent styling methods.
/// Derived classes must implement `style()` to return a mutable reference
/// to their StyleRefinement.
///
/// Usage:
/// ```cpp
/// class Div : public Styled<Div> {
///     StyleRefinement& style() { return style_; }
///     StyleRefinement style_;
/// };
///
/// auto elem = Div().flex().items_center().bg(red()).p(4_px);
/// ```
template <typename Derived>
class Styled {
public:
    // ─── Display ───────────────────────────────────────────────────────

    Derived& block() { self().style().display = Display::Block; return self(); }
    Derived& flex() { self().style().display = Display::Flex; return self(); }
    Derived& grid() { self().style().display = Display::Grid; return self(); }
    Derived& hidden() { self().style().display = Display::None; return self(); }
    Derived& visible() { self().style().visibility = Visibility::Visible; return self(); }
    Derived& invisible() { self().style().visibility = Visibility::Hidden; return self(); }

    // ─── Position ──────────────────────────────────────────────────────

    Derived& relative() { self().style().position = Position::Relative; return self(); }
    Derived& absolute() { self().style().position = Position::Absolute; return self(); }

    // ─── Overflow ──────────────────────────────────────────────────────

    Derived& overflow_hidden() {
        self().style().overflow_x = Overflow::Hidden;
        self().style().overflow_y = Overflow::Hidden;
        return self();
    }
    Derived& overflow_scroll() {
        self().style().overflow_x = Overflow::Scroll;
        self().style().overflow_y = Overflow::Scroll;
        return self();
    }
    Derived& overflow_x_scroll() { self().style().overflow_x = Overflow::Scroll; return self(); }
    Derived& overflow_y_scroll() { self().style().overflow_y = Overflow::Scroll; return self(); }

    // ─── Flex Direction ────────────────────────────────────────────────

    Derived& flex_row() { self().style().flex_direction = FlexDirection::Row; return self(); }
    Derived& flex_col() { self().style().flex_direction = FlexDirection::Column; return self(); }
    Derived& flex_row_reverse() { self().style().flex_direction = FlexDirection::RowReverse; return self(); }
    Derived& flex_col_reverse() { self().style().flex_direction = FlexDirection::ColumnReverse; return self(); }

    // ─── Flex Properties ───────────────────────────────────────────────

    Derived& flex_grow() { self().style().flex_grow = 1.0f; return self(); }
    Derived& flex_shrink() { self().style().flex_shrink = 1.0f; return self(); }
    Derived& flex_shrink_0() { self().style().flex_shrink = 0.0f; return self(); }
    Derived& flex_none() {
        self().style().flex_grow = 0.0f;
        self().style().flex_shrink = 0.0f;
        return self();
    }
    Derived& flex_1() {
        self().style().flex_grow = 1.0f;
        self().style().flex_shrink = 1.0f;
        self().style().flex_basis = Length(Pixels{0});
        return self();
    }
    Derived& flex_auto() {
        self().style().flex_grow = 1.0f;
        self().style().flex_shrink = 1.0f;
        self().style().flex_basis = Length::auto_();
        return self();
    }
    Derived& flex_wrap() { self().style().flex_wrap = FlexWrap::Wrap; return self(); }
    Derived& flex_nowrap() { self().style().flex_wrap = FlexWrap::NoWrap; return self(); }

    // ─── Alignment ─────────────────────────────────────────────────────

    Derived& items_start() { self().style().align_items = AlignItems::Start; return self(); }
    Derived& items_end() { self().style().align_items = AlignItems::End; return self(); }
    Derived& items_center() { self().style().align_items = AlignItems::Center; return self(); }
    Derived& items_stretch() { self().style().align_items = AlignItems::Stretch; return self(); }
    Derived& items_baseline() { self().style().align_items = AlignItems::Baseline; return self(); }

    Derived& justify_start() { self().style().justify_content = JustifyContent::Start; return self(); }
    Derived& justify_end() { self().style().justify_content = JustifyContent::End; return self(); }
    Derived& justify_center() { self().style().justify_content = JustifyContent::Center; return self(); }
    Derived& justify_between() { self().style().justify_content = JustifyContent::SpaceBetween; return self(); }
    Derived& justify_around() { self().style().justify_content = JustifyContent::SpaceAround; return self(); }
    Derived& justify_evenly() { self().style().justify_content = JustifyContent::SpaceEvenly; return self(); }

    Derived& self_start() { self().style().align_self = AlignSelf::Start; return self(); }
    Derived& self_end() { self().style().align_self = AlignSelf::End; return self(); }
    Derived& self_center() { self().style().align_self = AlignSelf::Center; return self(); }
    Derived& self_stretch() { self().style().align_self = AlignSelf::Stretch; return self(); }

    // ─── Sizing ────────────────────────────────────────────────────────

    Derived& w(Length len) { self().style().width = len; return self(); }
    Derived& h(Length len) { self().style().height = len; return self(); }
    Derived& w(Pixels px) { self().style().width = Length(px); return self(); }
    Derived& h(Pixels px) { self().style().height = Length(px); return self(); }
    Derived& w_full() { self().style().width = Length(DefiniteLength::from_fraction(1.0f)); return self(); }
    Derived& h_full() { self().style().height = Length(DefiniteLength::from_fraction(1.0f)); return self(); }
    Derived& w_auto() { self().style().width = Length::auto_(); return self(); }
    Derived& h_auto() { self().style().height = Length::auto_(); return self(); }

    Derived& min_w(Length len) { self().style().min_width = len; return self(); }
    Derived& min_h(Length len) { self().style().min_height = len; return self(); }
    Derived& max_w(Length len) { self().style().max_width = len; return self(); }
    Derived& max_h(Length len) { self().style().max_height = len; return self(); }

    Derived& size(Length len) {
        self().style().width = len;
        self().style().height = len;
        return self();
    }
    Derived& size(Pixels px) { return size(Length(px)); }

    // ─── Gap ───────────────────────────────────────────────────────────

    Derived& gap(DefiniteLength len) {
        self().style().gap = Size<DefiniteLength>{len, len};
        return self();
    }
    Derived& gap(Pixels px) { return gap(DefiniteLength(px)); }
    Derived& gap_x(DefiniteLength len) {
        if (!self().style().gap.has_value()) self().style().gap = Size<DefiniteLength>{};
        self().style().gap->width = len;
        return self();
    }
    Derived& gap_y(DefiniteLength len) {
        if (!self().style().gap.has_value()) self().style().gap = Size<DefiniteLength>{};
        self().style().gap->height = len;
        return self();
    }

    // ─── Padding ───────────────────────────────────────────────────────

    Derived& p(DefiniteLength len) {
        self().style().padding = Edges<DefiniteLength>{len, len, len, len};
        return self();
    }
    Derived& p(Pixels px) { return p(DefiniteLength(px)); }
    Derived& px(DefiniteLength len) {
        if (!self().style().padding.has_value()) self().style().padding = Edges<DefiniteLength>{};
        self().style().padding->left = len;
        self().style().padding->right = len;
        return self();
    }
    Derived& py(DefiniteLength len) {
        if (!self().style().padding.has_value()) self().style().padding = Edges<DefiniteLength>{};
        self().style().padding->top = len;
        self().style().padding->bottom = len;
        return self();
    }
    Derived& pt(DefiniteLength len) {
        if (!self().style().padding.has_value()) self().style().padding = Edges<DefiniteLength>{};
        self().style().padding->top = len;
        return self();
    }
    Derived& pb(DefiniteLength len) {
        if (!self().style().padding.has_value()) self().style().padding = Edges<DefiniteLength>{};
        self().style().padding->bottom = len;
        return self();
    }
    Derived& pl(DefiniteLength len) {
        if (!self().style().padding.has_value()) self().style().padding = Edges<DefiniteLength>{};
        self().style().padding->left = len;
        return self();
    }
    Derived& pr(DefiniteLength len) {
        if (!self().style().padding.has_value()) self().style().padding = Edges<DefiniteLength>{};
        self().style().padding->right = len;
        return self();
    }
    Derived& px(Pixels px_val) { return px(DefiniteLength(px_val)); }
    Derived& py(Pixels px_val) { return py(DefiniteLength(px_val)); }

    // ─── Margin ────────────────────────────────────────────────────────

    Derived& m(Length len) {
        self().style().margin = Edges<Length>{len, len, len, len};
        return self();
    }
    Derived& m_auto() { return m(Length::auto_()); }
    Derived& mx(Length len) {
        if (!self().style().margin.has_value()) self().style().margin = Edges<Length>{};
        self().style().margin->left = len;
        self().style().margin->right = len;
        return self();
    }
    Derived& my(Length len) {
        if (!self().style().margin.has_value()) self().style().margin = Edges<Length>{};
        self().style().margin->top = len;
        self().style().margin->bottom = len;
        return self();
    }
    Derived& mx_auto() { return mx(Length::auto_()); }

    // ─── Border ────────────────────────────────────────────────────────

    Derived& border(Pixels width = Pixels{1.0f}) {
        self().style().border_widths = Edges<Pixels>::all(width);
        self().style().border_style = BorderStyle::Solid;
        return self();
    }
    Derived& border_t(Pixels width = Pixels{1.0f}) {
        if (!self().style().border_widths.has_value()) self().style().border_widths = Edges<Pixels>{};
        self().style().border_widths->top = width;
        self().style().border_style = BorderStyle::Solid;
        return self();
    }
    Derived& border_b(Pixels width = Pixels{1.0f}) {
        if (!self().style().border_widths.has_value()) self().style().border_widths = Edges<Pixels>{};
        self().style().border_widths->bottom = width;
        self().style().border_style = BorderStyle::Solid;
        return self();
    }
    Derived& border_l(Pixels width = Pixels{1.0f}) {
        if (!self().style().border_widths.has_value()) self().style().border_widths = Edges<Pixels>{};
        self().style().border_widths->left = width;
        self().style().border_style = BorderStyle::Solid;
        return self();
    }
    Derived& border_r(Pixels width = Pixels{1.0f}) {
        if (!self().style().border_widths.has_value()) self().style().border_widths = Edges<Pixels>{};
        self().style().border_widths->right = width;
        self().style().border_style = BorderStyle::Solid;
        return self();
    }
    Derived& border_color(Rgba color) { self().style().border_color = color; return self(); }
    Derived& border_dashed() { self().style().border_style = BorderStyle::Dashed; return self(); }

    // ─── Corner Radius ─────────────────────────────────────────────────

    Derived& rounded(Pixels radius) {
        self().style().corner_radii = Corners<Pixels>::all(radius);
        return self();
    }
    Derived& rounded_sm() { return rounded(Pixels{2.0f}); }
    Derived& rounded_md() { return rounded(Pixels{4.0f}); }
    Derived& rounded_lg() { return rounded(Pixels{8.0f}); }
    Derived& rounded_xl() { return rounded(Pixels{12.0f}); }
    Derived& rounded_full() { return rounded(Pixels{9999.0f}); }
    Derived& rounded_none() { return rounded(Pixels{0.0f}); }

    Derived& rounded_tl(Pixels radius) {
        if (!self().style().corner_radii.has_value()) self().style().corner_radii = Corners<Pixels>{};
        self().style().corner_radii->top_left = radius;
        return self();
    }
    Derived& rounded_tr(Pixels radius) {
        if (!self().style().corner_radii.has_value()) self().style().corner_radii = Corners<Pixels>{};
        self().style().corner_radii->top_right = radius;
        return self();
    }
    Derived& rounded_bl(Pixels radius) {
        if (!self().style().corner_radii.has_value()) self().style().corner_radii = Corners<Pixels>{};
        self().style().corner_radii->bottom_left = radius;
        return self();
    }
    Derived& rounded_br(Pixels radius) {
        if (!self().style().corner_radii.has_value()) self().style().corner_radii = Corners<Pixels>{};
        self().style().corner_radii->bottom_right = radius;
        return self();
    }

    // ─── Background ────────────────────────────────────────────────────

    Derived& bg(Rgba color) { self().style().background = Background::solid(color); return self(); }
    Derived& bg(Background bg) { self().style().background = std::move(bg); return self(); }

    // ─── Opacity ───────────────────────────────────────────────────────

    Derived& opacity(float value) { self().style().opacity = value; return self(); }

    // ─── Shadow ────────────────────────────────────────────────────────

    Derived& shadow(BoxShadow shadow) {
        if (!self().style().box_shadow.has_value()) {
            self().style().box_shadow = std::vector<BoxShadow>{};
        }
        self().style().box_shadow->push_back(std::move(shadow));
        return self();
    }
    Derived& shadow_sm() {
        return shadow(BoxShadow{
            Rgba{0, 0, 0, 0.05f}, {Pixels{0}, Pixels{1}}, Pixels{2}, Pixels{0}
        });
    }
    Derived& shadow_md() {
        return shadow(BoxShadow{
            Rgba{0, 0, 0, 0.1f}, {Pixels{0}, Pixels{4}}, Pixels{6}, Pixels{-1}
        });
    }
    Derived& shadow_lg() {
        return shadow(BoxShadow{
            Rgba{0, 0, 0, 0.1f}, {Pixels{0}, Pixels{10}}, Pixels{15}, Pixels{-3}
        });
    }

    // ─── Cursor ────────────────────────────────────────────────────────

    Derived& cursor(CursorStyle c) { self().style().mouse_cursor = c; return self(); }
    Derived& cursor_pointer() { return cursor(CursorStyle::PointingHand); }
    Derived& cursor_text() { return cursor(CursorStyle::IBeam); }
    Derived& cursor_move() { return cursor(CursorStyle::OpenHand); }
    Derived& cursor_not_allowed() { return cursor(CursorStyle::OperationNotAllowed); }

    // ─── Text ──────────────────────────────────────────────────────────

    Derived& text_color(Rgba color) {
        ensure_text().color = color;
        return self();
    }
    Derived& text_size(AbsoluteLength size) {
        ensure_text().font_size = size;
        return self();
    }
    Derived& text_size(Pixels px) { return text_size(AbsoluteLength(px)); }
    Derived& text_size(Rems rem) { return text_size(AbsoluteLength(rem)); }

    Derived& text_xs() { return text_size(Rems{0.75f}); }
    Derived& text_sm() { return text_size(Rems{0.875f}); }
    Derived& text_base() { return text_size(Rems{1.0f}); }
    Derived& text_lg() { return text_size(Rems{1.125f}); }
    Derived& text_xl() { return text_size(Rems{1.25f}); }
    Derived& text_2xl() { return text_size(Rems{1.5f}); }
    Derived& text_3xl() { return text_size(Rems{1.875f}); }

    Derived& font_weight(FontWeight weight) {
        ensure_text().font_weight = weight;
        return self();
    }
    Derived& font_bold() { return font_weight(FontWeight::bold()); }
    Derived& font_medium() { return font_weight(FontWeight::medium()); }
    Derived& font_normal() { return font_weight(FontWeight::normal()); }
    Derived& font_light() { return font_weight(FontWeight::light()); }

    Derived& italic() {
        ensure_text().font_style = FontStyle::Italic;
        return self();
    }

    Derived& font_family(std::string family) {
        ensure_text().font_family = std::move(family);
        return self();
    }

    Derived& underline() {
        ensure_text().decoration = TextDecoration{TextDecoration::Line::Underline};
        return self();
    }
    Derived& line_through() {
        ensure_text().decoration = TextDecoration{TextDecoration::Line::Strikethrough};
        return self();
    }

    Derived& truncate() {
        self().style().overflow_x = Overflow::Hidden;
        ensure_text().overflow = TextOverflow::Ellipsis;
        return self();
    }

    Derived& line_height(float multiplier) {
        ensure_text().line_height = multiplier;
        return self();
    }

    // ─── Grid ──────────────────────────────────────────────────────────

    Derived& grid_cols(std::vector<GridTrack> cols) {
        self().style().grid_cols = std::move(cols);
        return self();
    }
    Derived& grid_rows(std::vector<GridTrack> rows) {
        self().style().grid_rows = std::move(rows);
        return self();
    }

private:
    Derived& self() { return static_cast<Derived&>(*this); }
    const Derived& self() const { return static_cast<const Derived&>(*this); }

    TextStyle& ensure_text() {
        if (!self().style().text.has_value()) {
            self().style().text = TextStyle{};
        }
        return *self().style().text;
    }
};

} // namespace gpui
