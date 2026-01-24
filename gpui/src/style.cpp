#include <gpui/style.h>

namespace gpui {

void Style::apply(const StyleRefinement& r) {
    if (r.display) display = *r.display;
    if (r.visibility) visibility = *r.visibility;
    if (r.position) position = *r.position;
    if (r.overflow_x) overflow_x = *r.overflow_x;
    if (r.overflow_y) overflow_y = *r.overflow_y;

    if (r.width) width = *r.width;
    if (r.height) height = *r.height;
    if (r.min_width) min_width = *r.min_width;
    if (r.min_height) min_height = *r.min_height;
    if (r.max_width) max_width = *r.max_width;
    if (r.max_height) max_height = *r.max_height;
    if (r.aspect_ratio) aspect_ratio = *r.aspect_ratio;

    if (r.margin) margin = *r.margin;
    if (r.padding) padding = *r.padding;
    if (r.inset) inset = *r.inset;

    if (r.flex_direction) flex_direction = *r.flex_direction;
    if (r.flex_wrap) flex_wrap = *r.flex_wrap;
    if (r.flex_grow) flex_grow = *r.flex_grow;
    if (r.flex_shrink) flex_shrink = *r.flex_shrink;
    if (r.flex_basis) flex_basis = *r.flex_basis;
    if (r.gap) gap = *r.gap;

    if (r.align_items) align_items = *r.align_items;
    if (r.align_self) align_self = *r.align_self;
    if (r.align_content) align_content = *r.align_content;
    if (r.justify_content) justify_content = *r.justify_content;

    if (r.border_widths) border_widths = *r.border_widths;
    if (r.border_color) border_color = *r.border_color;
    if (r.border_style) border_style = *r.border_style;
    if (r.corner_radii) corner_radii = *r.corner_radii;

    if (r.background) background = *r.background;
    if (r.opacity) opacity = *r.opacity;
    if (r.box_shadow) box_shadow = *r.box_shadow;
    if (r.mouse_cursor) mouse_cursor = *r.mouse_cursor;

    if (r.text) text.refine(*r.text);

    if (r.grid_cols) grid_cols = *r.grid_cols;
    if (r.grid_rows) grid_rows = *r.grid_rows;
    if (r.grid_location) grid_location = *r.grid_location;
}

} // namespace gpui
