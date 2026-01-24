#pragma once

// layout.h - A C++ port of SwiftUI's Layout protocol.
//
// Core protocol (two-pass layout):
//   1. Parent receives a ProposedViewSize from its parent
//   2. Parent's Layout::sizeThatFits queries children via subview.sizeThatFits(proposal)
//   3. Parent returns its own size
//   4. Parent's Layout::placeSubviews places children via subview.place(at, anchor, proposal)
//
// Key differences from Compose-style layout:
//   - Proposals are optional single values per axis (nil = ideal, 0 = min, inf = max)
//   - Measurement and placement are separate passes
//   - Alignment guides provide per-view customizable alignment points
//   - ViewSpacing encodes preferred inter-view spacing
//   - Layouts can maintain a cache across measurement/placement
//
// Usage:
//   auto root = VStack({.spacing = 8}, {
//       Leaf({100, 40}),
//       HStack({.spacing = 4}, {
//           Leaf({50, 30}),
//           Leaf({50, 30}),
//       }),
//   });
//   root->layout(ProposedViewSize(200, 400));
//   // read positions via root->collectBounds()
//   freeTree(root);

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>
#include <vector>

namespace swiftui {

static constexpr float Inf = std::numeric_limits<float>::infinity();

// ============================================================================
// Size
// ============================================================================

struct Size {
    float width = 0;
    float height = 0;
};

// ============================================================================
// Point
// ============================================================================

struct Point {
    float x = 0;
    float y = 0;
};

// ============================================================================
// Rect
// ============================================================================

struct Rect {
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;

    float minX() const { return x; }
    float minY() const { return y; }
    float maxX() const { return x + width; }
    float maxY() const { return y + height; }
    float midX() const { return x + width / 2; }
    float midY() const { return y + height / 2; }
};

// ============================================================================
// UnitPoint
// ============================================================================

// A point in the unit coordinate space [0,1] x [0,1], used as an anchor
// for placement. (0,0) = top-leading, (1,1) = bottom-trailing.
struct UnitPoint {
    float x = 0;
    float y = 0;

    static constexpr UnitPoint topLeading() { return {0, 0}; }
    static constexpr UnitPoint top() { return {0.5f, 0}; }
    static constexpr UnitPoint topTrailing() { return {1, 0}; }
    static constexpr UnitPoint leading() { return {0, 0.5f}; }
    static constexpr UnitPoint center() { return {0.5f, 0.5f}; }
    static constexpr UnitPoint trailing() { return {1, 0.5f}; }
    static constexpr UnitPoint bottomLeading() { return {0, 1}; }
    static constexpr UnitPoint bottom() { return {0.5f, 1}; }
    static constexpr UnitPoint bottomTrailing() { return {1, 1}; }
};

// ============================================================================
// Alignment
// ============================================================================

enum class HorizontalAlignment { leading, center, trailing };
enum class VerticalAlignment { top, center, bottom, firstTextBaseline, lastTextBaseline };

struct Alignment {
    HorizontalAlignment horizontal = HorizontalAlignment::center;
    VerticalAlignment vertical = VerticalAlignment::center;

    static constexpr Alignment topLeading() {
        return {HorizontalAlignment::leading, VerticalAlignment::top};
    }
    static constexpr Alignment top() {
        return {HorizontalAlignment::center, VerticalAlignment::top};
    }
    static constexpr Alignment topTrailing() {
        return {HorizontalAlignment::trailing, VerticalAlignment::top};
    }
    static constexpr Alignment leading() {
        return {HorizontalAlignment::leading, VerticalAlignment::center};
    }
    static constexpr Alignment center() {
        return {HorizontalAlignment::center, VerticalAlignment::center};
    }
    static constexpr Alignment trailing() {
        return {HorizontalAlignment::trailing, VerticalAlignment::center};
    }
    static constexpr Alignment bottomLeading() {
        return {HorizontalAlignment::leading, VerticalAlignment::bottom};
    }
    static constexpr Alignment bottom() {
        return {HorizontalAlignment::center, VerticalAlignment::bottom};
    }
    static constexpr Alignment bottomTrailing() {
        return {HorizontalAlignment::trailing, VerticalAlignment::bottom};
    }
};

// Compute the offset for a horizontal alignment within a given width.
float alignH(HorizontalAlignment align, float childWidth, float parentWidth);

// Compute the offset for a vertical alignment within a given height.
float alignV(VerticalAlignment align, float childHeight, float parentHeight);

// ============================================================================
// ProposedViewSize
// ============================================================================

// A size proposal from parent to child. Each dimension is independently optional:
//   - nullopt: child should return its ideal size
//   - 0: child should return its minimum size
//   - Inf: child should return its maximum size
//   - concrete value: child should fit within that dimension
struct ProposedViewSize {
    std::optional<float> width;
    std::optional<float> height;

    ProposedViewSize() = default;
    ProposedViewSize(std::optional<float> w, std::optional<float> h) : width(w), height(h) {}
    explicit ProposedViewSize(Size s) : width(s.width), height(s.height) {}
    explicit ProposedViewSize(float w, float h) : width(w), height(h) {}

    static ProposedViewSize zero() { return ProposedViewSize(0.0f, 0.0f); }
    static ProposedViewSize unspecified() { return ProposedViewSize(); }
    static ProposedViewSize infinity() { return ProposedViewSize(Inf, Inf); }

    // Replace nil dimensions with a fallback (default 10x10, matching SwiftUI).
    Size replacingUnspecifiedDimensions(Size fallback = {10, 10}) const {
        return {width.value_or(fallback.width), height.value_or(fallback.height)};
    }
};

// ============================================================================
// ViewDimensions
// ============================================================================

// The measured size of a view plus its alignment guide values.
struct ViewDimensions {
    float width = 0;
    float height = 0;

    // Explicit overrides (set via alignmentGuide on the view).
    std::optional<float> explicitLeading;
    std::optional<float> explicitHCenter;
    std::optional<float> explicitTrailing;
    std::optional<float> explicitTop;
    std::optional<float> explicitVCenter;
    std::optional<float> explicitBottom;

    // Get the resolved alignment guide value (explicit override or default).
    float operator[](HorizontalAlignment guide) const;
    float operator[](VerticalAlignment guide) const;
};

// ============================================================================
// Axis
// ============================================================================

enum class Axis { horizontal, vertical };

// ============================================================================
// ViewSpacing
// ============================================================================

// Encodes a view's preferred spacing relative to adjacent views.
struct ViewSpacing {
    float top = 0;
    float leading = 0;
    float bottom = 0;
    float trailing = 0;

    static ViewSpacing zero() { return {0, 0, 0, 0}; }

    // Preferred distance between this view and the next along an axis.
    float distance(const ViewSpacing& next, Axis axis) const;

    // Merge another spacing (take the max on each edge).
    void formUnion(const ViewSpacing& other);
};

// ============================================================================
// Forward declarations
// ============================================================================

class LayoutNode;
class Layout;

// ============================================================================
// LayoutSubview
// ============================================================================

// A proxy for a child view within a Layout. Provides measurement and placement.
class LayoutSubview {
    LayoutNode* node_ = nullptr;

public:
    LayoutSubview() = default;
    explicit LayoutSubview(LayoutNode* node) : node_(node) {}

    // Query the child's preferred size for a given proposal.
    Size sizeThatFits(ProposedViewSize proposal) const;

    // Query the child's dimensions (size + alignment guides) for a proposal.
    ViewDimensions dimensions(ProposedViewSize proposal) const;

    // Place the child at a position, with an anchor point and size proposal.
    void place(Point position, UnitPoint anchor, ProposedViewSize proposal) const;

    // Place at position with default anchor (topLeading) and given proposal.
    void place(Point position, ProposedViewSize proposal) const {
        place(position, UnitPoint::topLeading(), proposal);
    }

    // The view's layout priority (higher = gets space first in stacks).
    float priority() const;

    // The view's spacing preferences.
    ViewSpacing spacing() const;

    LayoutNode* node() const { return node_; }
};

// ============================================================================
// LayoutSubviews
// ============================================================================

// A random-access collection of LayoutSubview proxies.
struct LayoutSubviews {
    int len = 0;
    LayoutSubview* els = nullptr;

    LayoutSubviews() = default;
    LayoutSubviews(int len, LayoutSubview* els) : len(len), els(els) {}

    LayoutSubview* begin() const { return els; }
    LayoutSubview* end() const { return els + len; }
    bool empty() const { return len == 0; }
    int size() const { return len; }
    LayoutSubview& operator[](int i) const { return els[i]; }
};

// ============================================================================
// Layout (abstract base class — the "protocol")
// ============================================================================

// The Layout protocol equivalent. Subclass this to create custom layouts.
//
// Required overrides:
//   - sizeThatFits: determine the container's size given a proposal and subviews
//   - placeSubviews: position each subview within the resolved bounds
//
// Optional overrides:
//   - makeCache / destroyCache: per-node cache for expensive computations
//   - spacing: the container's preferred spacing relative to siblings
class Layout {
public:
    virtual ~Layout() = default;

    // Determine how much space this layout needs.
    virtual Size sizeThatFits(
        ProposedViewSize proposal,
        LayoutSubviews subviews,
        void* cache) = 0;

    // Place subviews within the resolved bounds.
    virtual void placeSubviews(
        Rect bounds,
        ProposedViewSize proposal,
        LayoutSubviews subviews,
        void* cache) = 0;

    // Create a layout cache. Default returns nullptr (no cache).
    virtual void* makeCache(LayoutSubviews subviews);

    // Destroy a cache created by makeCache. Default does nothing.
    virtual void destroyCache(void* cache);

    // The container's preferred spacing. Default unions all subview spacings.
    virtual ViewSpacing spacing(LayoutSubviews subviews, void* cache);
};

// ============================================================================
// LayoutNode
// ============================================================================

class LayoutNode {
    Layout* layout_;
    int childCount_ = 0;
    LayoutNode** children_ = nullptr;

    // Per-node state set during layout.
    float width_ = 0;
    float height_ = 0;
    float x_ = 0;
    float y_ = 0;
    void* cache_ = nullptr;

    // Per-node view properties.
    float priority_ = 0;
    ViewSpacing spacing_;
    ViewDimensions dims_;

    // Ideal size (used when proposal is unspecified).
    Size idealSize_ = {10, 10};

public:
    explicit LayoutNode(Layout* layout, std::initializer_list<LayoutNode*> children = {});
    ~LayoutNode();

    // Run the full layout pass: measure with the proposal, then place at (0,0).
    void layout(ProposedViewSize proposal);

    // Measure this node for a given proposal (returns its size).
    Size measure(ProposedViewSize proposal);

    // Place this node at a given origin (sets x_, y_).
    void placeAt(float x, float y);

    // Run the placement pass on this node's children (called after placeAt).
    void doPlace(ProposedViewSize proposal);

    // Accessors.
    float width() const { return width_; }
    float height() const { return height_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float priority() const { return priority_; }
    ViewSpacing spacing() const { return spacing_; }
    ViewDimensions dimensions() const { return dims_; }
    Size idealSize() const { return idealSize_; }

    // Setters for view properties.
    LayoutNode* setPriority(float p) { priority_ = p; return this; }
    LayoutNode* setSpacing(ViewSpacing s) { spacing_ = s; return this; }
    LayoutNode* setIdealSize(Size s) { idealSize_ = s; return this; }
    LayoutNode* setAlignmentGuide(HorizontalAlignment guide, float value);
    LayoutNode* setAlignmentGuide(VerticalAlignment guide, float value);

    // Collect the absolute bounds of all leaf nodes (for testing/debugging).
    struct BoundsRect { float x, y, width, height; };
    void collectBounds(std::vector<BoundsRect>& out, float offsetX = 0, float offsetY = 0) const;

    int childCount() const { return childCount_; }
    LayoutNode** children() const { return children_; }
};

void freeTree(LayoutNode* node);

// ============================================================================
// LeafLayout
// ============================================================================

// A leaf view with a fixed size (analogous to a fixed-frame view).
struct LeafConfig {
    float width = 0;
    float height = 0;
};

class LeafLayout : public Layout {
    LeafConfig config_;
public:
    explicit LeafLayout(LeafConfig config) : config_(config) {}
    Size sizeThatFits(ProposedViewSize proposal, LayoutSubviews subviews, void* cache) override;
    void placeSubviews(Rect bounds, ProposedViewSize proposal,
                       LayoutSubviews subviews, void* cache) override;
};

// ============================================================================
// FlexibleLeafLayout
// ============================================================================

// A leaf that responds to proposals: returns min/ideal/max depending on the proposal.
struct FlexibleLeafConfig {
    Size minSize = {0, 0};
    Size idealSize = {10, 10};
    Size maxSize = {Inf, Inf};
};

class FlexibleLeafLayout : public Layout {
    FlexibleLeafConfig config_;
public:
    explicit FlexibleLeafLayout(FlexibleLeafConfig config) : config_(config) {}
    Size sizeThatFits(ProposedViewSize proposal, LayoutSubviews subviews, void* cache) override;
    void placeSubviews(Rect bounds, ProposedViewSize proposal,
                       LayoutSubviews subviews, void* cache) override;
};

// ============================================================================
// HStackLayout
// ============================================================================

struct HStackConfig {
    VerticalAlignment alignment = VerticalAlignment::center;
    std::optional<float> spacing;  // nullopt = use ViewSpacing between adjacent views
};

class HStackLayout : public Layout {
    HStackConfig config_;
public:
    explicit HStackLayout(HStackConfig config = {}) : config_(config) {}
    Size sizeThatFits(ProposedViewSize proposal, LayoutSubviews subviews, void* cache) override;
    void placeSubviews(Rect bounds, ProposedViewSize proposal,
                       LayoutSubviews subviews, void* cache) override;
};

// ============================================================================
// VStackLayout
// ============================================================================

struct VStackConfig {
    HorizontalAlignment alignment = HorizontalAlignment::center;
    std::optional<float> spacing;  // nullopt = use ViewSpacing between adjacent views
};

class VStackLayout : public Layout {
    VStackConfig config_;
public:
    explicit VStackLayout(VStackConfig config = {}) : config_(config) {}
    Size sizeThatFits(ProposedViewSize proposal, LayoutSubviews subviews, void* cache) override;
    void placeSubviews(Rect bounds, ProposedViewSize proposal,
                       LayoutSubviews subviews, void* cache) override;
};

// ============================================================================
// ZStackLayout
// ============================================================================

struct ZStackConfig {
    Alignment alignment = Alignment::center();
};

class ZStackLayout : public Layout {
    ZStackConfig config_;
public:
    explicit ZStackLayout(ZStackConfig config = {}) : config_(config) {}
    Size sizeThatFits(ProposedViewSize proposal, LayoutSubviews subviews, void* cache) override;
    void placeSubviews(Rect bounds, ProposedViewSize proposal,
                       LayoutSubviews subviews, void* cache) override;
};

// ============================================================================
// Builder functions
// ============================================================================

LayoutNode* Leaf(LeafConfig config);
LayoutNode* Leaf(float width, float height);
LayoutNode* FlexibleLeaf(FlexibleLeafConfig config);
LayoutNode* HStack(HStackConfig config, std::initializer_list<LayoutNode*> children);
LayoutNode* VStack(VStackConfig config, std::initializer_list<LayoutNode*> children);
LayoutNode* ZStack(ZStackConfig config, std::initializer_list<LayoutNode*> children);
LayoutNode* HStack(std::initializer_list<LayoutNode*> children);
LayoutNode* VStack(std::initializer_list<LayoutNode*> children);
LayoutNode* ZStack(std::initializer_list<LayoutNode*> children);

} // namespace swiftui
