// layout_demo.cpp - Demonstrates the Compose-style layout engine.
//
// Build: clang++ -std=c++20 -o layout_demo layout.cpp layout_demo.cpp
//    or: g++ -std=c++20 -o layout_demo layout.cpp layout_demo.cpp

#include "layout.h"

#include <cstdio>

using namespace layout;

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

    auto root = Row({.spacing = 4}, {
        Leaf({.width = 50, .height = 30}),
        Leaf({.width = 60, .height = 40}),
        Leaf({.width = 70, .height = 20}),
    });

    root->measure(Constraints::unbounded());
    root->placeAt(0, 0);

    printf("  root size: %d x %d\n", root->width(), root->height());
    printBounds(*root);

    assert_eq(root->width(), 188, "row width");
    assert_eq(root->height(), 40, "row height");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 2: Column with vertical arrangement
// ============================================================================
static void example_column() {
    printf("=== Column with 3 leaves (spacing=8) ===\n");

    auto root = Column({.spacing = 8}, {
        Leaf({.width = 100, .height = 30}),
        Leaf({.width = 80, .height = 40}),
        Leaf({.width = 120, .height = 25}),
    });

    root->measure(Constraints::unbounded());
    root->placeAt(0, 0);

    printf("  root size: %d x %d\n", root->width(), root->height());
    printBounds(*root);

    assert_eq(root->width(), 120, "column width");
    assert_eq(root->height(), 111, "column height");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 3: Box (stacking) with center alignment
// ============================================================================
static void example_box() {
    printf("=== Box (center aligned) ===\n");

    auto root = Box(
        {.horizontalAlignment = Alignment::Center,
         .verticalAlignment = Alignment::Center},
        {
            Leaf({.width = 200, .height = 200}),
            Leaf({.width = 80, .height = 40}),
        });

    root->measure(Constraints::unbounded());
    root->placeAt(0, 0);

    printf("  root size: %d x %d\n", root->width(), root->height());
    printBounds(*root);

    assert_eq(root->width(), 200, "box width");
    assert_eq(root->height(), 200, "box height");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 4: Nested layout - Column containing Rows
// ============================================================================
static void example_nested() {
    printf("=== Nested: Column { Row, Row } ===\n");

    auto root = Column({.spacing = 10}, {
        Row({.spacing = 5}, {
            Leaf({.width = 40, .height = 30}),
            Leaf({.width = 60, .height = 30}),
        }),
        Row({.spacing = 5}, {
            Leaf({.width = 80, .height = 25}),
            Leaf({.width = 30, .height = 25}),
        }),
    });

    root->measure(Constraints::unbounded());
    root->placeAt(0, 0);

    printf("  root size: %d x %d\n", root->width(), root->height());
    printBounds(*root);

    assert_eq(root->width(), 115, "nested width");
    assert_eq(root->height(), 65, "nested height");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 5: Constrained layout - fixed parent width
// ============================================================================
static void example_constrained() {
    printf("=== Row with fixed parent width (100) ===\n");

    auto root = Row({.spacing = 4}, {
        Leaf({.width = 50, .height = 30}),
        Leaf({.width = 80, .height = 30}),
    });

    root->measure(Constraints::fixedWidth(100));
    root->placeAt(0, 0);

    printf("  root size: %d x %d\n", root->width(), root->height());
    printBounds(*root);

    assert_eq(root->width(), 100, "constrained row width");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 6: Row with cross-axis alignment
// ============================================================================
static void example_cross_alignment() {
    printf("=== Row with Center cross-axis alignment ===\n");

    auto root = Row(
        {.spacing = 4, .crossAxisAlignment = Alignment::Center},
        {
            Leaf({.width = 40, .height = 20}),
            Leaf({.width = 40, .height = 60}),
            Leaf({.width = 40, .height = 30}),
        });

    root->measure(Constraints::unbounded());
    root->placeAt(0, 0);

    printf("  root size: %d x %d\n", root->width(), root->height());
    printBounds(*root);

    assert_eq(root->height(), 60, "cross-align row height");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 7: SpaceBetween arrangement
// ============================================================================
static void example_space_between() {
    printf("=== Row with SpaceBetween in 300px ===\n");

    auto root = Row(
        {.arrangement = Arrangement::SpaceBetween},
        {
            Leaf({.width = 50, .height = 30}),
            Leaf({.width = 50, .height = 30}),
            Leaf({.width = 50, .height = 30}),
        });

    root->measure(Constraints::fixed(300, 30));
    root->placeAt(0, 0);

    printf("  root size: %d x %d\n", root->width(), root->height());
    printBounds(*root);

    assert_eq(root->width(), 300, "space-between width");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 8: Custom MeasurePolicy
// ============================================================================

class DiagonalPolicy : public MeasurePolicy {
    int step_;

public:
    explicit DiagonalPolicy(int step = 20) : step_(step) {}

    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override {
        int offsetX = 0;
        int offsetY = 0;
        int maxRight = 0;
        int maxBottom = 0;

        for (auto* child : children) {
            child->measure(constraints.loosen());
            child->placeAt(offsetX, offsetY);
            maxRight = std::max(maxRight, offsetX + child->width());
            maxBottom = std::max(maxBottom, offsetY + child->height());
            offsetX += step_;
            offsetY += step_;
        }

        return {constraints.constrainWidth(maxRight),
                constraints.constrainHeight(maxBottom)};
    }
};

static void example_custom_policy() {
    printf("=== Custom policy: diagonal layout ===\n");

    auto root = Layout(new DiagonalPolicy(20), {
        Leaf({.width = 40, .height = 40}),
        Leaf({.width = 40, .height = 40}),
        Leaf({.width = 40, .height = 40}),
    });

    root->measure(Constraints::unbounded());
    root->placeAt(0, 0);

    printf("  root size: %d x %d\n", root->width(), root->height());
    printBounds(*root);

    assert_eq(root->width(), 80, "diagonal width");
    assert_eq(root->height(), 80, "diagonal height");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 9: FlowRow - wrapping horizontal layout
// ============================================================================
static void example_flow_row() {
    printf("=== FlowRow in 100px width (spacing=4, crossSpacing=6) ===\n");

    // 5 items of 40px each: can fit 2 per line in 100px (40+4+40=84 <= 100)
    // Line 1: items 0,1 (width=84, height=20)
    // Line 2: items 2,3 (width=84, height=20)
    // Line 3: item 4   (width=40, height=20)
    auto root = FlowRow(
        {.mainAxisSpacing = 4, .crossAxisSpacing = 6},
        {
            Leaf({.width = 40, .height = 20}),
            Leaf({.width = 40, .height = 20}),
            Leaf({.width = 40, .height = 20}),
            Leaf({.width = 40, .height = 20}),
            Leaf({.width = 40, .height = 20}),
        });

    root->measure(Constraints::fixedWidth(100));
    root->placeAt(0, 0);

    printf("  root size: %d x %d\n", root->width(), root->height());
    printBounds(*root);

    // 3 lines: heights 20+6+20+6+20 = 72
    assert_eq(root->width(), 100, "flow row width");
    assert_eq(root->height(), 72, "flow row height");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 10: FlowRow with varying child sizes
// ============================================================================
static void example_flow_row_varying() {
    printf("=== FlowRow varying sizes in 120px ===\n");

    // Children: 50, 60, 40, 70, 30
    // Line 1: 50+4+60 = 114 <= 120 => items 0,1
    // Line 2: 40+4+70 = 114 <= 120 => items 2,3
    // Line 3: 30 => item 4
    auto root = FlowRow(
        {.mainAxisSpacing = 4, .crossAxisSpacing = 2},
        {
            Leaf({.width = 50, .height = 25}),
            Leaf({.width = 60, .height = 30}),
            Leaf({.width = 40, .height = 20}),
            Leaf({.width = 70, .height = 35}),
            Leaf({.width = 30, .height = 15}),
        });

    root->measure(Constraints::fixedWidth(120));
    root->placeAt(0, 0);

    printf("  root size: %d x %d\n", root->width(), root->height());
    printBounds(*root);

    // Line heights: max(25,30)=30, max(20,35)=35, 15
    // Total: 30+2+35+2+15 = 84
    assert_eq(root->width(), 120, "flow row varying width");
    assert_eq(root->height(), 84, "flow row varying height");
    printf("\n");
    freeTree(root);
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
    example_flow_row();
    example_flow_row_varying();

    printf("All examples complete.\n");
    return 0;
}
