#pragma once

// compose_layout.h - A C++ layout engine inspired by Jetpack Compose's layout protocol.
//
// Core protocol:
//   1. Parent receives Constraints from its parent
//   2. Parent's MeasurePolicy measures children: child->measure(childConstraints)
//   3. Parent reads child->width() / child->height() for measured sizes
//   4. Parent places children: child->placeAt(x, y)
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
//   root->placeAt(0, 0);
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
// MeasureResult, MeasurePolicy
// ============================================================================

struct MeasureResult {
    int width = 0;
    int height = 0;
};

class MeasurePolicy {
public:
    virtual ~MeasurePolicy() = default;
    virtual MeasureResult Measure(LayoutNodeVec children, Constraints constraints) = 0;

    // Intrinsic measurement queries. These allow a parent to ask a child's
    // preferred size before the actual measurement pass, without violating
    // the single-measure rule.
    virtual int MinIntrinsicWidth(LayoutNodeVec children, int height);
    virtual int MaxIntrinsicWidth(LayoutNodeVec children, int height);
    virtual int MinIntrinsicHeight(LayoutNodeVec children, int width);
    virtual int MaxIntrinsicHeight(LayoutNodeVec children, int width);
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
    bool matchParentSize_ = false;

public:
    explicit LayoutNode(MeasurePolicy* policy, LayoutNodeVec children = {});
    ~LayoutNode();

    LayoutNode* measure(Constraints constraints);
    void placeAt(int x, int y) { x_ = x; y_ = y; }

    // Intrinsic measurement queries.
    int minIntrinsicWidth(int height);
    int maxIntrinsicWidth(int height);
    int minIntrinsicHeight(int width);
    int maxIntrinsicHeight(int width);

    // Weight support for Row/Column proportional sizing.
    // weight > 0 means this child participates in weighted distribution.
    // fill=true: child gets exact constraints (must fill allocation).
    // fill=false: child gets bounded constraints (can be smaller).
    LayoutNode* setWeight(float w, bool fill = true) { weight_ = w; fillWeight_ = fill; return this; }
    float weight() const { return weight_; }
    bool fillWeight() const { return fillWeight_; }

    // matchParentSize: within a Box, this child does not influence the Box's
    // own size but instead receives exact constraints equal to the Box's
    // resolved size (determined by other children).
    LayoutNode* setMatchParentSize(bool v = true) { matchParentSize_ = v; return this; }
    bool matchParentSize() const { return matchParentSize_; }

    int width() const { return measuredWidth_; }
    int height() const { return measuredHeight_; }
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
    int MinIntrinsicWidth(LayoutNodeVec children, int height) override;
    int MaxIntrinsicWidth(LayoutNodeVec children, int height) override;
    int MinIntrinsicHeight(LayoutNodeVec children, int width) override;
    int MaxIntrinsicHeight(LayoutNodeVec children, int width) override;
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
    int MinIntrinsicWidth(LayoutNodeVec children, int height) override;
    int MaxIntrinsicWidth(LayoutNodeVec children, int height) override;
    int MinIntrinsicHeight(LayoutNodeVec children, int width) override;
    int MaxIntrinsicHeight(LayoutNodeVec children, int width) override;
};

struct BoxConfig {
    Alignment horizontalAlignment = Alignment::Start;
    Alignment verticalAlignment = Alignment::Start;
    // When true, passes parent min constraints through to children.
    // When false (default), loosens constraints (sets min to 0).
    bool propagateMinConstraints = false;
};

class BoxPolicy : public MeasurePolicy {
    BoxConfig config_;
public:
    explicit BoxPolicy(BoxConfig config = {}) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
    int MinIntrinsicWidth(LayoutNodeVec children, int height) override;
    int MaxIntrinsicWidth(LayoutNodeVec children, int height) override;
    int MinIntrinsicHeight(LayoutNodeVec children, int width) override;
    int MaxIntrinsicHeight(LayoutNodeVec children, int width) override;
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
    int MinIntrinsicWidth(LayoutNodeVec children, int height) override;
    int MaxIntrinsicWidth(LayoutNodeVec children, int height) override;
    int MinIntrinsicHeight(LayoutNodeVec children, int width) override;
    int MaxIntrinsicHeight(LayoutNodeVec children, int width) override;
};

enum class FlowOverflow {
    Visible,  // All items are laid out even beyond maxLines (no clipping)
    Clip,     // Items beyond maxLines are not placed
};

struct FlowRowConfig {
    int mainAxisSpacing = 0;
    int crossAxisSpacing = 0;
    int maxItemsInEachRow = 0;  // 0 = unlimited
    int maxLines = 0;           // 0 = unlimited
    FlowOverflow overflow = FlowOverflow::Clip;
    Alignment crossAxisAlignment = Alignment::Start;
};

class FlowRowPolicy : public MeasurePolicy {
    FlowRowConfig config_;
public:
    explicit FlowRowPolicy(FlowRowConfig config = {}) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

// Spacer: takes exact size if constraints are fixed, collapses to 0 otherwise.
// Entirely driven by parent constraints or sizing modifiers.
class SpacerPolicy : public MeasurePolicy {
public:
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
    int MinIntrinsicWidth(LayoutNodeVec children, int height) override;
    int MaxIntrinsicWidth(LayoutNodeVec children, int height) override;
    int MinIntrinsicHeight(LayoutNodeVec children, int width) override;
    int MaxIntrinsicHeight(LayoutNodeVec children, int width) override;
};

struct FlowColumnConfig {
    int mainAxisSpacing = 0;
    int crossAxisSpacing = 0;
    int maxItemsInEachColumn = 0;  // 0 = unlimited
    int maxColumns = 0;            // 0 = unlimited
    FlowOverflow overflow = FlowOverflow::Clip;
    Alignment crossAxisAlignment = Alignment::Start;
};

class FlowColumnPolicy : public MeasurePolicy {
    FlowColumnConfig config_;
public:
    explicit FlowColumnPolicy(FlowColumnConfig config = {}) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

// ============================================================================
// LayoutNodeProvider (for lazy layouts)
// ============================================================================

// Interface for deferred node creation. LazyColumn/LazyRow use this to create
// only the nodes that are visible in the viewport, avoiding measurement of
// off-screen items. The provider acts as a factory: get() creates or returns
// a cached node for the given index. Ownership of returned nodes transfers to
// the lazy policy (they are freed on re-measure or policy destruction).
class LayoutNodeProvider {
public:
    virtual ~LayoutNodeProvider() = default;
    virtual int count() = 0;
    virtual LayoutNode* get(int index) = 0;
};

// ============================================================================
// LazyColumn / LazyRow policies
// ============================================================================

struct LazyColumnConfig {
    int spacing = 0;
    Alignment crossAxisAlignment = Alignment::Start;
};

class LazyColumnPolicy : public MeasurePolicy {
    LazyColumnConfig config_;
    LayoutNodeProvider* provider_;
    int firstVisibleIndex_ = 0;
    int firstVisibleItemOffset_ = 0;
    std::vector<LayoutNode*> composedNodes_;

public:
    explicit LazyColumnPolicy(LayoutNodeProvider* provider, LazyColumnConfig config = {});
    ~LazyColumnPolicy() override;

    void scrollTo(int index, int offset = 0) {
        firstVisibleIndex_ = index;
        firstVisibleItemOffset_ = offset;
    }
    int firstVisibleIndex() const { return firstVisibleIndex_; }
    int firstVisibleItemOffset() const { return firstVisibleItemOffset_; }
    LayoutNodeProvider* provider() const { return provider_; }
    const std::vector<LayoutNode*>& composedNodes() const { return composedNodes_; }

    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

struct LazyRowConfig {
    int spacing = 0;
    Alignment crossAxisAlignment = Alignment::Start;
};

class LazyRowPolicy : public MeasurePolicy {
    LazyRowConfig config_;
    LayoutNodeProvider* provider_;
    int firstVisibleIndex_ = 0;
    int firstVisibleItemOffset_ = 0;
    std::vector<LayoutNode*> composedNodes_;

public:
    explicit LazyRowPolicy(LayoutNodeProvider* provider, LazyRowConfig config = {});
    ~LazyRowPolicy() override;

    void scrollTo(int index, int offset = 0) {
        firstVisibleIndex_ = index;
        firstVisibleItemOffset_ = offset;
    }
    int firstVisibleIndex() const { return firstVisibleIndex_; }
    int firstVisibleItemOffset() const { return firstVisibleItemOffset_; }
    LayoutNodeProvider* provider() const { return provider_; }
    const std::vector<LayoutNode*>& composedNodes() const { return composedNodes_; }

    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

// ============================================================================
// GridCells strategy
// ============================================================================

// Determines how columns (or rows) are sized in a lazy grid.
// Fixed(n): exactly n columns of equal width.
// Adaptive(minSize): as many columns as fit, each at least minSize pixels.
// FixedSize(size): columns of exact pixel size, as many as fit.
struct GridCells {
    enum class Type { Fixed, Adaptive, FixedSize };
    Type type = Type::Fixed;
    int value = 1;

    static constexpr GridCells Fixed(int count) { return {Type::Fixed, count}; }
    static constexpr GridCells Adaptive(int minSize) { return {Type::Adaptive, minSize}; }
    static constexpr GridCells FixedSize(int size) { return {Type::FixedSize, size}; }
};

// Computes column/row sizes from a GridCells strategy and available space.
std::vector<int> computeCellSizes(GridCells cells, int availableSize, int spacing);

// ============================================================================
// LazyVerticalGrid / LazyHorizontalGrid policies
// ============================================================================

struct LazyVerticalGridConfig {
    GridCells columns = GridCells::Fixed(1);
    int mainAxisSpacing = 0;   // vertical spacing between rows
    int crossAxisSpacing = 0;  // horizontal spacing between columns
};

class LazyVerticalGridPolicy : public MeasurePolicy {
    LazyVerticalGridConfig config_;
    LayoutNodeProvider* provider_;
    int firstVisibleIndex_ = 0;
    int firstVisibleItemOffset_ = 0;
    std::vector<LayoutNode*> composedNodes_;

public:
    explicit LazyVerticalGridPolicy(LayoutNodeProvider* provider, LazyVerticalGridConfig config = {});
    ~LazyVerticalGridPolicy() override;

    void scrollTo(int index, int offset = 0) {
        firstVisibleIndex_ = index;
        firstVisibleItemOffset_ = offset;
    }
    int firstVisibleIndex() const { return firstVisibleIndex_; }
    int firstVisibleItemOffset() const { return firstVisibleItemOffset_; }
    LayoutNodeProvider* provider() const { return provider_; }
    const std::vector<LayoutNode*>& composedNodes() const { return composedNodes_; }

    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

struct LazyHorizontalGridConfig {
    GridCells rows = GridCells::Fixed(1);
    int mainAxisSpacing = 0;   // horizontal spacing between columns
    int crossAxisSpacing = 0;  // vertical spacing between rows
};

class LazyHorizontalGridPolicy : public MeasurePolicy {
    LazyHorizontalGridConfig config_;
    LayoutNodeProvider* provider_;
    int firstVisibleIndex_ = 0;
    int firstVisibleItemOffset_ = 0;
    std::vector<LayoutNode*> composedNodes_;

public:
    explicit LazyHorizontalGridPolicy(LayoutNodeProvider* provider, LazyHorizontalGridConfig config = {});
    ~LazyHorizontalGridPolicy() override;

    void scrollTo(int index, int offset = 0) {
        firstVisibleIndex_ = index;
        firstVisibleItemOffset_ = offset;
    }
    int firstVisibleIndex() const { return firstVisibleIndex_; }
    int firstVisibleItemOffset() const { return firstVisibleItemOffset_; }
    LayoutNodeProvider* provider() const { return provider_; }
    const std::vector<LayoutNode*>& composedNodes() const { return composedNodes_; }

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

// Attempts to size content to a given width/height ratio by trying constraint
// combinations in priority order. ratio = width / height.
struct AspectRatioConfig {
    float ratio = 1.0f;                   // width / height
    bool matchHeightConstraintsFirst = false;
};

class AspectRatioPolicy : public MeasurePolicy {
    AspectRatioConfig config_;
public:
    explicit AspectRatioPolicy(AspectRatioConfig config) : config_(config) {}
    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override;
};

// ============================================================================
// Builder functions
// ============================================================================

LayoutNode* Leaf(LeafConfig config);
LayoutNode* Spacer();
LayoutNode* Row(RowConfig config, LayoutNodeVec children);
LayoutNode* Column(ColumnConfig config, LayoutNodeVec children);
LayoutNode* Box(BoxConfig config, LayoutNodeVec children);
LayoutNode* FlowRow(FlowRowConfig config, LayoutNodeVec children);
LayoutNode* Row(LayoutNodeVec children);
LayoutNode* Column(LayoutNodeVec children);
LayoutNode* Box(LayoutNodeVec children);
LayoutNode* FlowRow(LayoutNodeVec children);
LayoutNode* FlowColumn(FlowColumnConfig config, LayoutNodeVec children);
LayoutNode* FlowColumn(LayoutNodeVec children);
LayoutNode* LazyColumn(LayoutNodeProvider* provider, LazyColumnConfig config = {});
LayoutNode* LazyRow(LayoutNodeProvider* provider, LazyRowConfig config = {});
LayoutNode* LazyVerticalGrid(LayoutNodeProvider* provider, LazyVerticalGridConfig config = {});
LayoutNode* LazyHorizontalGrid(LayoutNodeProvider* provider, LazyHorizontalGridConfig config = {});
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
LayoutNode* AspectRatio(LayoutNode* child, AspectRatioConfig config);
LayoutNode* AspectRatio(LayoutNode* child, float ratio);

} // namespace compose
