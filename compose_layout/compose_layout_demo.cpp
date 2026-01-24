// compose_layout_demo.cpp - Demonstrates the Compose-style layout engine.
//
// Build: clang++ -std=c++20 -o compose_layout_demo compose_layout_demo.cpp
//    or: g++ -std=c++20 -o compose_layout_demo compose_layout_demo.cpp

#include "compose_layout.h"

#include <cstdio>

using namespace compose;

static void printBounds(const LayoutNode& root) {
    std::vector<LayoutNode::Rect> rects;
    root.collectBounds(rects);
    for (size_t i = 0; i < rects.size(); i++) {
        auto& r = rects[i];
        printf("  leaf[%zu]: x=%d y=%d w=%d h=%d\n", i, r.x, r.y, r.width, r.height);
    }
}

static void assert_eq(int actual, int expected, const char* msg) {
    if (actual != expected) {
        printf("FAIL: %s: expected %d, got %d\n", msg, expected, actual);
    }
}

// ============================================================================
// Example 1: Simple Row with three fixed-size leaves
// ============================================================================
static void example_row() {
    printf("=== Row with 3 leaves (spacing=4) ===\n");

    auto root = Row(RowConfig{.spacing = 4},
        Leaf({.width = 50, .height = 30}),
        Leaf({.width = 60, .height = 40}),
        Leaf({.width = 70, .height = 20})
    );

    root->measure(Constraints::unbounded());
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    // Verify: total width = 50+60+70 + 4*2 = 188, height = max(30,40,20) = 40
    assert_eq(root->measuredWidth(), 188, "row width");
    assert_eq(root->measuredHeight(), 40, "row height");
    printf("\n");
}

// ============================================================================
// Example 2: Column with vertical arrangement
// ============================================================================
static void example_column() {
    printf("=== Column with 3 leaves (spacing=8) ===\n");

    auto root = Column(ColumnConfig{.spacing = 8},
        Leaf({.width = 100, .height = 30}),
        Leaf({.width = 80, .height = 40}),
        Leaf({.width = 120, .height = 25})
    );

    root->measure(Constraints::unbounded());
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    // width = max(100,80,120) = 120, height = 30+40+25 + 8*2 = 111
    assert_eq(root->measuredWidth(), 120, "column width");
    assert_eq(root->measuredHeight(), 111, "column height");
    printf("\n");
}

// ============================================================================
// Example 3: Box (stacking) with center alignment
// ============================================================================
static void example_box() {
    printf("=== Box (center aligned) ===\n");

    auto root = Box(
        BoxConfig{.horizontalAlignment = Alignment::Center,
                  .verticalAlignment = Alignment::Center},
        Leaf({.width = 200, .height = 200}),  // background
        Leaf({.width = 80, .height = 40})      // centered content
    );

    root->measure(Constraints::unbounded());
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    // Box size = max of children = 200x200
    // Second child centered: x=(200-80)/2=60, y=(200-40)/2=80
    assert_eq(root->measuredWidth(), 200, "box width");
    assert_eq(root->measuredHeight(), 200, "box height");
    printf("\n");
}

// ============================================================================
// Example 4: Nested layout - Column containing Rows
// ============================================================================
static void example_nested() {
    printf("=== Nested: Column { Row, Row } ===\n");

    auto root = Column(ColumnConfig{.spacing = 10},
        Row(RowConfig{.spacing = 5},
            Leaf({.width = 40, .height = 30}),
            Leaf({.width = 60, .height = 30})
        ),
        Row(RowConfig{.spacing = 5},
            Leaf({.width = 80, .height = 25}),
            Leaf({.width = 30, .height = 25})
        )
    );

    root->measure(Constraints::unbounded());
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    // Row 1: width=40+60+5=105, height=30
    // Row 2: width=80+30+5=115, height=25
    // Column: width=max(105,115)=115, height=30+25+10=65
    assert_eq(root->measuredWidth(), 115, "nested width");
    assert_eq(root->measuredHeight(), 65, "nested height");
    printf("\n");
}

// ============================================================================
// Example 5: Constrained layout - fixed parent width forces wrapping
// ============================================================================
static void example_constrained() {
    printf("=== Row with fixed parent width (100) ===\n");

    auto root = Row(RowConfig{.spacing = 4},
        Leaf({.width = 50, .height = 30}),
        Leaf({.width = 80, .height = 30})  // will be constrained
    );

    // Parent only allows 100px wide
    root->measure(Constraints::fixedWidth(100));
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    // With 100px total and spacing=4: first child gets 50, remaining = 100-4-50=46
    // Second child requested 80 but constrained to 46
    assert_eq(root->measuredWidth(), 100, "constrained row width");
    printf("\n");
}

// ============================================================================
// Example 6: Row with cross-axis alignment
// ============================================================================
static void example_cross_alignment() {
    printf("=== Row with Center cross-axis alignment ===\n");

    auto root = Row(
        RowConfig{.spacing = 4, .crossAxisAlignment = Alignment::Center},
        Leaf({.width = 40, .height = 20}),
        Leaf({.width = 40, .height = 60}),  // tallest
        Leaf({.width = 40, .height = 30})
    );

    root->measure(Constraints::unbounded());
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    // height = max(20,60,30) = 60
    // first child y = (60-20)/2 = 20
    // second child y = 0
    // third child y = (60-30)/2 = 15
    assert_eq(root->measuredHeight(), 60, "cross-align row height");
    printf("\n");
}

// ============================================================================
// Example 7: SpaceBetween arrangement
// ============================================================================
static void example_space_between() {
    printf("=== Row with SpaceBetween in 300px ===\n");

    auto root = Row(
        RowConfig{.arrangement = Arrangement::SpaceBetween},
        Leaf({.width = 50, .height = 30}),
        Leaf({.width = 50, .height = 30}),
        Leaf({.width = 50, .height = 30})
    );

    root->measure(Constraints::fixed(300, 30));
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    // 300 - 150 = 150 free space, divided into 2 gaps = 75 each
    // positions: 0, 50+75=125, 125+50+75=250
    assert_eq(root->measuredWidth(), 300, "space-between width");
    printf("\n");
}

// ============================================================================
// Example 8: Custom MeasurePolicy
// ============================================================================
static void example_custom_policy() {
    printf("=== Custom policy: diagonal layout ===\n");

    // A custom layout that places children diagonally
    auto diagonalPolicy = [](std::span<std::unique_ptr<LayoutNode>> children,
                             Constraints constraints) -> MeasureResult {
        int offsetX = 0;
        int offsetY = 0;
        int maxRight = 0;
        int maxBottom = 0;

        for (auto& child : children) {
            auto placeable = child->measure(constraints.loosen());
            placeable.placeAt(offsetX, offsetY);
            maxRight = std::max(maxRight, offsetX + placeable.width());
            maxBottom = std::max(maxBottom, offsetY + placeable.height());
            offsetX += 20;  // diagonal step
            offsetY += 20;
        }

        return {constraints.constrainWidth(maxRight),
                constraints.constrainHeight(maxBottom)};
    };

    auto root = Layout(diagonalPolicy,
        Leaf({.width = 40, .height = 40}),
        Leaf({.width = 40, .height = 40}),
        Leaf({.width = 40, .height = 40})
    );

    root->measure(Constraints::unbounded());
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    // 3 items at diagonal step 20: last starts at (40,40), size 40x40
    // total: 80x80
    assert_eq(root->measuredWidth(), 80, "diagonal width");
    assert_eq(root->measuredHeight(), 80, "diagonal height");
    printf("\n");
}

int main() {
    printf("Compose Layout Engine Demo\n");
    printf("==========================\n\n");

    example_row();
    example_column();
    example_box();
    example_nested();
    example_constrained();
    example_cross_alignment();
    example_space_between();
    example_custom_policy();

    printf("All examples complete.\n");
    return 0;
}
