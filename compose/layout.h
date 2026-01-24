#pragma once

// compose_layout.h - A C++ layout engine inspired by Jetpack Compose's layout protocol.
//
// Core protocol:
//   1. Parent receives Constraints from its parent
//   2. Parent's MeasurePolicy measures children: child.measure(childConstraints) -> Placeable
//   3. Parent determines its own size from children's measured sizes
//   4. Parent places children: placeable.placeAt(x, y)
//
// Usage:
//   auto root = Column({.spacing = 8}, {
//       Leaf({.width = 100, .height = 40}),
//       Row({.spacing = 4}, {
//           Leaf({.width = 50, .height = 30}),
//           Leaf({.width = 50, .height = 30}),
//       }),
//   });
//   root->measure(Constraints::fixed(200, 400));
//   root->place(0, 0);
//   freeTree(root);  // caller owns the memory

#include <algorithm>
#include <initializer_list>
#include <limits>
#include <span>
#include <vector>

namespace compose {

static constexpr int Infinity = std::numeric_limits<int>::max();

// ============================================================================
// Constraints
// ============================================================================

struct Constraints {
    int minWidth = 0;
    int maxWidth = Infinity;
    int minHeight = 0;
    int maxHeight = Infinity;

    static constexpr Constraints fixed(int width, int height) {
        return {width, width, height, height};
    }
    static constexpr Constraints unbounded() {
        return {0, Infinity, 0, Infinity};
    }
    static constexpr Constraints fixedWidth(int width) {
        return {width, width, 0, Infinity};
    }
    static constexpr Constraints fixedHeight(int height) {
        return {0, Infinity, height, height};
    }

    constexpr int constrainWidth(int width) const { return std::clamp(width, minWidth, maxWidth); }
    constexpr int constrainHeight(int height) const { return std::clamp(height, minHeight, maxHeight); }
    constexpr bool hasBoundedWidth() const { return maxWidth != Infinity; }
    constexpr bool hasBoundedHeight() const { return maxHeight != Infinity; }
    constexpr bool hasFixedWidth() const { return minWidth == maxWidth; }
    constexpr bool hasFixedHeight() const { return minHeight == maxHeight; }
    constexpr Constraints withMaxWidth(int w) const { return {std::min(minWidth, w), w, minHeight, maxHeight}; }
    constexpr Constraints withMaxHeight(int h) const { return {minWidth, maxWidth, std::min(minHeight, h), h}; }
    constexpr Constraints loosen() const { return {0, maxWidth, 0, maxHeight}; }
};

// ============================================================================
// LayoutNodeVec
// ============================================================================

class LayoutNode;

struct LayoutNodeVec {
    int len = 0;
    LayoutNode** els = nullptr;

    LayoutNodeVec() = default;
    LayoutNodeVec(int len, LayoutNode** els) : len(len), els(els) {}
    LayoutNodeVec(std::initializer_list<LayoutNode*> list);

    LayoutNode** begin() const { return els; }
    LayoutNode** end() const { return els + len; }
    bool empty() const { return len == 0; }
    int size() const { return len; }
    LayoutNode* operator[](int i) const { return els[i]; }
};

void freeVec(LayoutNodeVec& vec);

// ============================================================================
// MeasureResult, Placeable, MeasurePolicy
// ============================================================================

struct MeasureResult {
    int width = 0;
    int height = 0;
};

class Placeable {
    LayoutNode* node_;

public:
    explicit Placeable(LayoutNode* node) : node_(node) {}
    int width() const;
    int height() const;
    void placeAt(int x, int y);
    LayoutNode* node() const { return node_; }
};

class MeasurePolicy {
public:
    virtual ~MeasurePolicy() = default;
    virtual MeasureResult Measure(LayoutNodeVec children, Constraints constraints) = 0;
};

// ============================================================================
// LayoutNode
// ============================================================================

class LayoutNode {
    MeasurePolicy* policy_;
    LayoutNodeVec children_;

    int measuredWidth_ = 0;
    int measuredHeight_ = 0;
    int x_ = 0;
    int y_ = 0;
    bool measured_ = false;
    float weight_ = 0.0f;
    bool fillWeight_ = true;

public:
    explicit LayoutNode(MeasurePolicy* policy, LayoutNodeVec children = {});
    ~LayoutNode();

    Placeable measure(Constraints constraints);
    void place(int x, int y) { x_ = x; y_ = y; }

    // Weight support for Row/Column proportional sizing.
    // weight > 0 means this child participates in weighted distribution.
    // fill=true: child gets exact constraints (must fill allocation).
    // fill=false: child gets bounded constraints (can be smaller).
    LayoutNode* setWeight(float w, bool fill = true) { weight_ = w; fillWeight_ = fill; return this; }
    float weight() const { return weight_; }
    bool fillWeight() const { return fillWeight_; }

    int measuredWidth() const { return measuredWidth_; }
    int measuredHeight() const { return measuredHeight_; }
    int x() const { return x_; }
    int y() const { return y_; }
    bool isMeasured() const { return measured_; }
    LayoutNodeVec children() const { return children_; }

    struct Rect { int x, y, width, height; };
    void collectBounds(std::vector<Rect>& out, int offsetX = 0, int offsetY = 0) const;
};

void freeTree(LayoutNode* node);

// ============================================================================
// Arrangement & Alignment
// ============================================================================

enum class Arrangement {
    Start, End, Center, SpaceBetween, SpaceAround, SpaceEvenly,
};

enum class Alignment {
    Start, Center, End,
};

std::vector<int> arrange(Arrangement arrangement, int totalSize,
                         std::span<const int> childSizes, int spacing = 0);
int align(Alignment alignment, int childSize, int parentSize);

// ============================================================================
// Policy classes
// ============================================================================

struct RowConfig {
    int spacing = 0;
    Arrangement arrangement = Arrangement::Start;
    Alignment crossAxisAlignment = Alignment::Start;
};

class RowPolicy : public MeasurePolicy {
    RowConfig config_;
public:
    explicit RowPolicy(RowConfig config = {}) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

struct ColumnConfig {
    int spacing = 0;
    Arrangement arrangement = Arrangement::Start;
    Alignment crossAxisAlignment = Alignment::Start;
};

class ColumnPolicy : public MeasurePolicy {
    ColumnConfig config_;
public:
    explicit ColumnPolicy(ColumnConfig config = {}) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

struct BoxConfig {
    Alignment horizontalAlignment = Alignment::Start;
    Alignment verticalAlignment = Alignment::Start;
};

class BoxPolicy : public MeasurePolicy {
    BoxConfig config_;
public:
    explicit BoxPolicy(BoxConfig config = {}) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

struct LeafConfig {
    int width = 0;
    int height = 0;
};

class LeafPolicy : public MeasurePolicy {
    LeafConfig config_;
public:
    explicit LeafPolicy(LeafConfig config) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

struct FlowRowConfig {
    int mainAxisSpacing = 0;
    int crossAxisSpacing = 0;
    int maxItemsInEachRow = 0;  // 0 = unlimited
    Alignment crossAxisAlignment = Alignment::Start;
};

class FlowRowPolicy : public MeasurePolicy {
    FlowRowConfig config_;
public:
    explicit FlowRowPolicy(FlowRowConfig config = {}) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

// ============================================================================
// Sizing modifier policies (constraint transformers)
// ============================================================================

// Makes constraints exact: min=max*fraction. If max is unbounded, has no effect.
// A negative fraction means "don't affect this axis".
struct FillMaxSizeConfig {
    float widthFraction = 1.0f;
    float heightFraction = 1.0f;
};

class FillMaxSizePolicy : public MeasurePolicy {
    FillMaxSizeConfig config_;
public:
    explicit FillMaxSizePolicy(FillMaxSizeConfig config = {}) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

// Sets fixed constraints in specified dimensions. -1 means "don't constrain".
struct SizeConfig {
    int width = -1;
    int height = -1;
};

class SizePolicy : public MeasurePolicy {
    SizeConfig config_;
public:
    explicit SizePolicy(SizeConfig config) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

// Like SizePolicy but overrides parent constraints entirely.
// The child is centered within the parent's allocated space if it differs.
struct RequiredSizeConfig {
    int width = -1;
    int height = -1;
};

class RequiredSizePolicy : public MeasurePolicy {
    RequiredSizeConfig config_;
public:
    explicit RequiredSizePolicy(RequiredSizeConfig config) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

// Resets min constraints to 0 (loosens). If unbounded=true, also sets max to Infinity.
struct WrapContentConfig {
    bool unbounded = false;
    Alignment horizontalAlignment = Alignment::Center;
    Alignment verticalAlignment = Alignment::Center;
};

class WrapContentPolicy : public MeasurePolicy {
    WrapContentConfig config_;
public:
    explicit WrapContentPolicy(WrapContentConfig config = {}) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

// Sets a minimum only if the incoming constraints don't already specify one.
struct DefaultMinSizeConfig {
    int minWidth = 0;
    int minHeight = 0;
};

class DefaultMinSizePolicy : public MeasurePolicy {
    DefaultMinSizeConfig config_;
public:
    explicit DefaultMinSizePolicy(DefaultMinSizeConfig config) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

// Clamps constraints to given ranges.
struct SizeInConfig {
    int minWidth = 0;
    int maxWidth = Infinity;
    int minHeight = 0;
    int maxHeight = Infinity;
};

class SizeInPolicy : public MeasurePolicy {
    SizeInConfig config_;
public:
    explicit SizeInPolicy(SizeInConfig config) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

// ============================================================================
// Builder functions
// ============================================================================

LayoutNode* Leaf(LeafConfig config);
LayoutNode* Row(RowConfig config, LayoutNodeVec children);
LayoutNode* Column(ColumnConfig config, LayoutNodeVec children);
LayoutNode* Box(BoxConfig config, LayoutNodeVec children);
LayoutNode* FlowRow(FlowRowConfig config, LayoutNodeVec children);
LayoutNode* Row(LayoutNodeVec children);
LayoutNode* Column(LayoutNodeVec children);
LayoutNode* Box(LayoutNodeVec children);
LayoutNode* FlowRow(LayoutNodeVec children);
LayoutNode* Layout(MeasurePolicy* policy, LayoutNodeVec children = {});

// Sizing modifier wrappers - each wraps a single child and transforms constraints.
LayoutNode* FillMaxSize(LayoutNode* child, FillMaxSizeConfig config = {});
LayoutNode* FillMaxWidth(LayoutNode* child, float fraction = 1.0f);
LayoutNode* FillMaxHeight(LayoutNode* child, float fraction = 1.0f);
LayoutNode* Size(LayoutNode* child, int width, int height);
LayoutNode* Width(LayoutNode* child, int width);
LayoutNode* Height(LayoutNode* child, int height);
LayoutNode* RequiredSize(LayoutNode* child, int width, int height);
LayoutNode* WrapContent(LayoutNode* child, WrapContentConfig config = {});
LayoutNode* DefaultMinSize(LayoutNode* child, DefaultMinSizeConfig config);
LayoutNode* SizeIn(LayoutNode* child, SizeInConfig config);

} // namespace compose
