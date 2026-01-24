// layout_demo.cpp - Demonstrates the SwiftUI-style layout engine.
//
// Build: clang++ -std=c++20 -o layout_demo layout.cpp layout_demo.cpp
//    or: g++ -std=c++20 -o layout_demo layout.cpp layout_demo.cpp

#include "layout.h"

#include <cmath>
#include <cstdio>

using namespace swiftui;

static void printBounds(const LayoutNode& root) {
    std::vector<LayoutNode::BoundsRect> rects;
    root.collectBounds(rects);
    for (size_t i = 0; i < rects.size(); i++) {
        auto& r = rects[i];
        printf("  leaf[%zu]: x=%.1f y=%.1f w=%.1f h=%.1f\n",
               i, r.x, r.y, r.width, r.height);
    }
}

static int failures = 0;

static void assert_near(float actual, float expected, const char* msg, float eps = 0.1f) {
    if (std::fabs(actual - expected) > eps) {
        printf("FAIL: %s: expected %.1f, got %.1f\n", msg, expected, actual);
        failures++;
    }
}

// ============================================================================
// Example 1: Simple HStack with three fixed-size leaves
// ============================================================================
static void example_hstack() {
    printf("=== HStack with 3 leaves (spacing=4) ===\n");

    auto* root = HStack({.spacing = 4}, {
        Leaf(50, 30),
        Leaf(60, 40),
        Leaf(70, 20),
    });

    root->layout(ProposedViewSize::unspecified());

    printf("  root size: %.1f x %.1f\n", root->width(), root->height());
    printBounds(*root);

    // Total width: 50 + 4 + 60 + 4 + 70 = 188
    // Max height: 40
    assert_near(root->width(), 188, "hstack width");
    assert_near(root->height(), 40, "hstack height");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 2: VStack with vertical arrangement
// ============================================================================
static void example_vstack() {
    printf("=== VStack with 3 leaves (spacing=8) ===\n");

    auto* root = VStack({.spacing = 8}, {
        Leaf(100, 30),
        Leaf(80, 40),
        Leaf(120, 25),
    });

    root->layout(ProposedViewSize::unspecified());

    printf("  root size: %.1f x %.1f\n", root->width(), root->height());
    printBounds(*root);

    // Max width: 120
    // Total height: 30 + 8 + 40 + 8 + 25 = 111
    assert_near(root->width(), 120, "vstack width");
    assert_near(root->height(), 111, "vstack height");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 3: ZStack (overlapping) with center alignment
// ============================================================================
static void example_zstack() {
    printf("=== ZStack (center aligned) ===\n");

    auto* root = ZStack({.alignment = Alignment::center()}, {
        Leaf(100, 80),
        Leaf(50, 40),
    });

    root->layout(ProposedViewSize::unspecified());

    printf("  root size: %.1f x %.1f\n", root->width(), root->height());
    printBounds(*root);

    // ZStack size: max of children = 100x80
    // Second child centered: (100-50)/2=25, (80-40)/2=20
    assert_near(root->width(), 100, "zstack width");
    assert_near(root->height(), 80, "zstack height");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 4: Nested layout (VStack containing HStacks)
// ============================================================================
static void example_nested() {
    printf("=== Nested: VStack { HStack, HStack } (spacing=10) ===\n");

    auto* root = VStack({.spacing = 10}, {
        HStack({.spacing = 5}, {
            Leaf(40, 20),
            Leaf(60, 20),
        }),
        HStack({.spacing = 5}, {
            Leaf(30, 25),
            Leaf(30, 25),
            Leaf(30, 25),
        }),
    });

    root->layout(ProposedViewSize::unspecified());

    printf("  root size: %.1f x %.1f\n", root->width(), root->height());
    printBounds(*root);

    // Top row: 40+5+60 = 105 wide, 20 tall
    // Bottom row: 30+5+30+5+30 = 100 wide, 25 tall
    // VStack: max width = 105, total height = 20+10+25 = 55
    assert_near(root->width(), 105, "nested width");
    assert_near(root->height(), 55, "nested height");
    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 5: ProposedViewSize semantics with FlexibleLeaf
// ============================================================================
static void example_flexible() {
    printf("=== FlexibleLeaf responds to proposals ===\n");

    FlexibleLeafConfig config = {
        .minSize = {20, 10},
        .idealSize = {100, 50},
        .maxSize = {300, 200},
    };

    auto* node = FlexibleLeaf(config);

    // Unspecified proposal -> ideal size
    Size ideal = node->measure(ProposedViewSize::unspecified());
    printf("  unspecified -> %.1f x %.1f\n", ideal.width, ideal.height);
    assert_near(ideal.width, 100, "flex ideal width");
    assert_near(ideal.height, 50, "flex ideal height");

    // Zero proposal -> minimum size
    Size minimum = node->measure(ProposedViewSize::zero());
    printf("  zero        -> %.1f x %.1f\n", minimum.width, minimum.height);
    assert_near(minimum.width, 20, "flex min width");
    assert_near(minimum.height, 10, "flex min height");

    // Infinity proposal -> maximum size
    Size maximum = node->measure(ProposedViewSize::infinity());
    printf("  infinity    -> %.1f x %.1f\n", maximum.width, maximum.height);
    assert_near(maximum.width, 300, "flex max width");
    assert_near(maximum.height, 200, "flex max height");

    // Concrete proposal -> clamped
    Size clamped = node->measure(ProposedViewSize(150, 75));
    printf("  (150, 75)   -> %.1f x %.1f\n", clamped.width, clamped.height);
    assert_near(clamped.width, 150, "flex clamped width");
    assert_near(clamped.height, 75, "flex clamped height");

    printf("\n");
    freeTree(node);
}

// ============================================================================
// Example 6: Alignment in VStack
// ============================================================================
static void example_alignment() {
    printf("=== VStack with .leading alignment ===\n");

    auto* root = VStack({.alignment = HorizontalAlignment::leading, .spacing = 0}, {
        Leaf(100, 30),
        Leaf(50, 30),
        Leaf(75, 30),
    });

    root->layout(ProposedViewSize::unspecified());
    printBounds(*root);

    // All children should be left-aligned (x=0).
    std::vector<LayoutNode::BoundsRect> rects;
    root->collectBounds(rects);
    assert_near(rects[0].x, 0, "leading align child 0");
    assert_near(rects[1].x, 0, "leading align child 1");
    assert_near(rects[2].x, 0, "leading align child 2");

    printf("\n");
    freeTree(root);

    printf("=== VStack with .trailing alignment ===\n");

    root = VStack({.alignment = HorizontalAlignment::trailing, .spacing = 0}, {
        Leaf(100, 30),
        Leaf(50, 30),
        Leaf(75, 30),
    });

    root->layout(ProposedViewSize::unspecified());
    printBounds(*root);

    rects.clear();
    root->collectBounds(rects);
    // All children should be right-aligned: x = parentWidth - childWidth
    assert_near(rects[0].x, 0, "trailing align child 0 (widest)");
    assert_near(rects[1].x, 50, "trailing align child 1");
    assert_near(rects[2].x, 25, "trailing align child 2");

    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 7: Priority-based space distribution
// ============================================================================
static void example_priority() {
    printf("=== HStack with priority (constrained width) ===\n");

    // Two flexible children in a constrained HStack.
    // Child 0 has higher priority -> gets space first.
    auto* root = HStack({.spacing = 0}, {
        FlexibleLeaf({.minSize = {10, 30}, .idealSize = {100, 30}, .maxSize = {200, 30}}),
        FlexibleLeaf({.minSize = {10, 30}, .idealSize = {100, 30}, .maxSize = {200, 30}}),
    });
    root->children()[0]->setPriority(1);

    root->layout(ProposedViewSize(150, 30));

    printf("  root size: %.1f x %.1f\n", root->width(), root->height());
    printBounds(*root);

    // With 150px available and priority(1) on child 0:
    // Priority determines measurement order. Child 0 measured first, gets 150/2 = 75.
    // Child 1 gets remaining 75/1 = 75. Both flexible children share equally here.
    std::vector<LayoutNode::BoundsRect> rects;
    root->collectBounds(rects);
    printf("  child[0] width: %.1f (priority=1)\n", rects[0].width);
    printf("  child[1] width: %.1f (priority=0)\n", rects[1].width);

    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 8: ViewSpacing (automatic spacing from subviews)
// ============================================================================
static void example_view_spacing() {
    printf("=== VStack with nil spacing (uses ViewSpacing) ===\n");

    // Set custom spacing on children.
    auto* child0 = Leaf(80, 30);
    child0->setSpacing({.top = 0, .leading = 0, .bottom = 12, .trailing = 0});
    auto* child1 = Leaf(80, 30);
    child1->setSpacing({.top = 8, .leading = 0, .bottom = 8, .trailing = 0});
    auto* child2 = Leaf(80, 30);
    child2->setSpacing({.top = 4, .leading = 0, .bottom = 0, .trailing = 0});

    // spacing = nullopt -> use ViewSpacing::distance between adjacent pairs.
    auto* root = VStack({.alignment = HorizontalAlignment::center, .spacing = std::nullopt}, {
        child0, child1, child2,
    });

    root->layout(ProposedViewSize::unspecified());

    printf("  root size: %.1f x %.1f\n", root->width(), root->height());
    printBounds(*root);

    // Spacing 0-1: max(child0.bottom=12, child1.top=8) = 12
    // Spacing 1-2: max(child1.bottom=8, child2.top=4) = 8
    // Total height: 30 + 12 + 30 + 8 + 30 = 110
    assert_near(root->height(), 110, "view spacing total height");

    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 9: Custom Layout (radial/circular layout)
// ============================================================================

class RadialLayout : public Layout {
    float radius_;
public:
    explicit RadialLayout(float radius) : radius_(radius) {}

    Size sizeThatFits(ProposedViewSize /*proposal*/, LayoutSubviews subviews,
                      void* /*cache*/) override {
        // The container is a square bounding the circle.
        float maxChildSize = 0;
        for (auto& sv : subviews) {
            Size sz = sv.sizeThatFits(ProposedViewSize::unspecified());
            maxChildSize = std::max(maxChildSize, std::max(sz.width, sz.height));
        }
        float diameter = 2 * radius_ + maxChildSize;
        return {diameter, diameter};
    }

    void placeSubviews(Rect bounds, ProposedViewSize /*proposal*/,
                       LayoutSubviews subviews, void* /*cache*/) override {
        int n = subviews.size();
        if (n == 0) return;

        float centerX = bounds.midX();
        float centerY = bounds.midY();

        for (int i = 0; i < n; i++) {
            float angle = (2.0f * 3.14159265f * i) / n;
            float x = centerX + radius_ * std::cos(angle);
            float y = centerY + radius_ * std::sin(angle);

            Size sz = subviews[i].sizeThatFits(ProposedViewSize::unspecified());
            ProposedViewSize childProposal(sz.width, sz.height);
            subviews[i].place({x, y}, UnitPoint::center(), childProposal);
        }
    }
};

static void example_custom_layout() {
    printf("=== Custom RadialLayout (radius=50) ===\n");

    auto* root = new LayoutNode(new RadialLayout(50), {
        Leaf(20, 20),
        Leaf(20, 20),
        Leaf(20, 20),
        Leaf(20, 20),
    });

    root->layout(ProposedViewSize::unspecified());

    printf("  root size: %.1f x %.1f\n", root->width(), root->height());
    printBounds(*root);

    // Container: 2*50 + 20 = 120 x 120
    assert_near(root->width(), 120, "radial width");
    assert_near(root->height(), 120, "radial height");

    printf("\n");
    freeTree(root);
}

// ============================================================================
// Example 10: Anchor-based placement
// ============================================================================
static void example_anchor_placement() {
    printf("=== ZStack with anchor placement ===\n");

    // Custom layout that places a child at the center using .center anchor.
    class CenterPlaceLayout : public Layout {
    public:
        Size sizeThatFits(ProposedViewSize proposal, LayoutSubviews /*subviews*/,
                          void* /*cache*/) override {
            auto sz = proposal.replacingUnspecifiedDimensions({200, 200});
            return sz;
        }
        void placeSubviews(Rect bounds, ProposedViewSize /*proposal*/,
                           LayoutSubviews subviews, void* /*cache*/) override {
            for (auto& sv : subviews) {
                Size sz = sv.sizeThatFits(ProposedViewSize::unspecified());
                // Place child center at bounds center.
                sv.place({bounds.midX(), bounds.midY()}, UnitPoint::center(),
                         ProposedViewSize(sz.width, sz.height));
            }
        }
    };

    auto* root = new LayoutNode(new CenterPlaceLayout(), {
        Leaf(40, 30),
    });

    root->layout(ProposedViewSize(200, 200));

    printBounds(*root);

    std::vector<LayoutNode::BoundsRect> rects;
    root->collectBounds(rects);
    // Child (40x30) centered in 200x200: x=80, y=85
    assert_near(rects[0].x, 80, "anchor x");
    assert_near(rects[0].y, 85, "anchor y");

    printf("\n");
    freeTree(root);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("SwiftUI Layout Engine - C++ Port Demo\n");
    printf("======================================\n\n");

    example_hstack();
    example_vstack();
    example_zstack();
    example_nested();
    example_flexible();
    example_alignment();
    example_priority();
    example_view_spacing();
    example_custom_layout();
    example_anchor_placement();

    if (failures == 0) {
        printf("All assertions passed.\n");
    } else {
        printf("%d assertion(s) FAILED.\n", failures);
    }
    return failures;
}
