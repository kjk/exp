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

    auto root = Row({.spacing = 4}, {
        Leaf({.width = 50, .height = 30}),
        Leaf({.width = 60, .height = 40}),
        Leaf({.width = 70, .height = 20}),
    });

    root->measure(Constraints::unbounded());
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    assert_eq(root->measuredWidth(), 188, "row width");
    assert_eq(root->measuredHeight(), 40, "row height");
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
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    assert_eq(root->measuredWidth(), 120, "column width");
    assert_eq(root->measuredHeight(), 111, "column height");
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
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    assert_eq(root->measuredWidth(), 200, "box width");
    assert_eq(root->measuredHeight(), 200, "box height");
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
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    assert_eq(root->measuredWidth(), 115, "nested width");
    assert_eq(root->measuredHeight(), 65, "nested height");
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
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    assert_eq(root->measuredWidth(), 100, "constrained row width");
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
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    assert_eq(root->measuredHeight(), 60, "cross-align row height");
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
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    assert_eq(root->measuredWidth(), 300, "space-between width");
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

    MeasureResult Measure(std::span<LayoutNode*> children, Constraints constraints) override {
        int offsetX = 0;
        int offsetY = 0;
        int maxRight = 0;
        int maxBottom = 0;

        for (auto* child : children) {
            auto placeable = child->measure(constraints.loosen());
            placeable.placeAt(offsetX, offsetY);
            maxRight = std::max(maxRight, offsetX + placeable.width());
            maxBottom = std::max(maxBottom, offsetY + placeable.height());
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
    root->place(0, 0);

    printf("  root size: %d x %d\n", root->measuredWidth(), root->measuredHeight());
    printBounds(*root);

    assert_eq(root->measuredWidth(), 80, "diagonal width");
    assert_eq(root->measuredHeight(), 80, "diagonal height");
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

    printf("All examples complete.\n");
    return 0;
}
