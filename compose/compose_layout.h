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
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <numeric>
#include <span>
#include <vector>

namespace compose {

// ============================================================================
// Core value types
// ============================================================================

static constexpr int Infinity = std::numeric_limits<int>::max();

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

    constexpr int constrainWidth(int width) const {
        return std::clamp(width, minWidth, maxWidth);
    }

    constexpr int constrainHeight(int height) const {
        return std::clamp(height, minHeight, maxHeight);
    }

    constexpr bool hasBoundedWidth() const { return maxWidth != Infinity; }
    constexpr bool hasBoundedHeight() const { return maxHeight != Infinity; }
    constexpr bool hasFixedWidth() const { return minWidth == maxWidth; }
    constexpr bool hasFixedHeight() const { return minHeight == maxHeight; }

    constexpr Constraints withMaxWidth(int w) const {
        return {std::min(minWidth, w), w, minHeight, maxHeight};
    }

    constexpr Constraints withMaxHeight(int h) const {
        return {minWidth, maxWidth, std::min(minHeight, h), h};
    }

    constexpr Constraints loosen() const {
        return {0, maxWidth, 0, maxHeight};
    }
};

// ============================================================================
// LayoutNodeVec - lightweight array of LayoutNode pointers
// ============================================================================

class LayoutNode;

struct LayoutNodeVec {
    int len = 0;
    LayoutNode** els = nullptr;

    LayoutNodeVec() = default;

    LayoutNodeVec(int len, LayoutNode** els) : len(len), els(els) {}

    // Construct from initializer_list (allocates a copy)
    LayoutNodeVec(std::initializer_list<LayoutNode*> list)
        : len(static_cast<int>(list.size())),
          els(list.size() ? new LayoutNode*[list.size()] : nullptr) {
        int i = 0;
        for (auto* node : list) {
            els[i++] = node;
        }
    }

    LayoutNode** begin() const { return els; }
    LayoutNode** end() const { return els + len; }
    bool empty() const { return len == 0; }
    int size() const { return len; }
    LayoutNode* operator[](int i) const { return els[i]; }
};

// Free the els array (does not free the nodes themselves).
inline void freeVec(LayoutNodeVec& vec) {
    delete[] vec.els;
    vec.els = nullptr;
    vec.len = 0;
}

// ============================================================================
// Forward declarations
// ============================================================================

struct MeasureResult {
    int width = 0;
    int height = 0;
};

// Placeable: a non-owning handle to a measured LayoutNode.
class Placeable {
    LayoutNode* node_;

public:
    explicit Placeable(LayoutNode* node) : node_(node) {}

    int width() const;
    int height() const;
    void placeAt(int x, int y);
    LayoutNode* node() const { return node_; }
};

// ============================================================================
// MeasurePolicy - abstract base class for layout algorithms
// ============================================================================

class MeasurePolicy {
public:
    virtual ~MeasurePolicy() = default;
    virtual MeasureResult Measure(LayoutNodeVec children, Constraints constraints) = 0;
};

// ============================================================================
// LayoutNode - the core tree node
// ============================================================================

class LayoutNode {
    MeasurePolicy* policy_;
    LayoutNodeVec children_;

    // Measurement state
    int measuredWidth_ = 0;
    int measuredHeight_ = 0;
    int x_ = 0;
    int y_ = 0;
    bool measured_ = false;

public:
    explicit LayoutNode(MeasurePolicy* policy, LayoutNodeVec children = {})
        : policy_(policy), children_(children) {}

    ~LayoutNode() {
        delete policy_;
        delete[] children_.els;
    }

    Placeable measure(Constraints constraints) {
        if (policy_) {
            auto result = policy_->Measure(children_, constraints);
            measuredWidth_ = constraints.constrainWidth(result.width);
            measuredHeight_ = constraints.constrainHeight(result.height);
        }
        measured_ = true;
        return Placeable(this);
    }

    void place(int x, int y) {
        x_ = x;
        y_ = y;
    }

    int measuredWidth() const { return measuredWidth_; }
    int measuredHeight() const { return measuredHeight_; }
    int x() const { return x_; }
    int y() const { return y_; }
    bool isMeasured() const { return measured_; }

    LayoutNodeVec children() const { return children_; }

    struct Rect {
        int x, y, width, height;
    };

    void collectBounds(std::vector<Rect>& out, int offsetX = 0, int offsetY = 0) const {
        int absX = offsetX + x_;
        int absY = offsetY + y_;
        if (children_.empty()) {
            out.push_back({absX, absY, measuredWidth_, measuredHeight_});
        }
        for (auto* child : children_) {
            child->collectBounds(out, absX, absY);
        }
    }
};

inline int Placeable::width() const { return node_->measuredWidth(); }
inline int Placeable::height() const { return node_->measuredHeight(); }
inline void Placeable::placeAt(int x, int y) { node_->place(x, y); }

// Recursively delete a tree of LayoutNodes (destructor handles policy and children array).
inline void freeTree(LayoutNode* node) {
    if (!node) return;
    for (auto* child : node->children()) {
        freeTree(child);
    }
    delete node;
}

// ============================================================================
// Arrangement - strategies for distributing children along an axis
// ============================================================================

enum class Arrangement {
    Start,
    End,
    Center,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
};

inline std::vector<int> arrange(Arrangement arrangement, int totalSize,
                                std::span<const int> childSizes, int spacing = 0) {
    int n = static_cast<int>(childSizes.size());
    if (n == 0) return {};

    int childrenTotal = std::accumulate(childSizes.begin(), childSizes.end(), 0);
    int totalSpacing = spacing * (n - 1);
    int freeSpace = totalSize - childrenTotal - totalSpacing;

    std::vector<int> positions(n);

    switch (arrangement) {
    case Arrangement::Start: {
        int pos = 0;
        for (int i = 0; i < n; i++) {
            positions[i] = pos;
            pos += childSizes[i] + spacing;
        }
        break;
    }
    case Arrangement::End: {
        int pos = std::max(0, freeSpace);
        for (int i = 0; i < n; i++) {
            positions[i] = pos;
            pos += childSizes[i] + spacing;
        }
        break;
    }
    case Arrangement::Center: {
        int pos = std::max(0, freeSpace / 2);
        for (int i = 0; i < n; i++) {
            positions[i] = pos;
            pos += childSizes[i] + spacing;
        }
        break;
    }
    case Arrangement::SpaceBetween: {
        if (n == 1) {
            positions[0] = 0;
        } else {
            int gap = (totalSize - childrenTotal) / (n - 1);
            int pos = 0;
            for (int i = 0; i < n; i++) {
                positions[i] = pos;
                pos += childSizes[i] + gap;
            }
        }
        break;
    }
    case Arrangement::SpaceAround: {
        int gap = (totalSize - childrenTotal) / n;
        int pos = gap / 2;
        for (int i = 0; i < n; i++) {
            positions[i] = pos;
            pos += childSizes[i] + gap;
        }
        break;
    }
    case Arrangement::SpaceEvenly: {
        int gap = (totalSize - childrenTotal) / (n + 1);
        int pos = gap;
        for (int i = 0; i < n; i++) {
            positions[i] = pos;
            pos += childSizes[i] + gap;
        }
        break;
    }
    }

    return positions;
}

// ============================================================================
// Alignment - cross-axis positioning
// ============================================================================

enum class Alignment {
    Start,
    Center,
    End,
};

inline int align(Alignment alignment, int childSize, int parentSize) {
    switch (alignment) {
    case Alignment::Start: return 0;
    case Alignment::Center: return (parentSize - childSize) / 2;
    case Alignment::End: return parentSize - childSize;
    }
    return 0;
}

// ============================================================================
// Built-in MeasurePolicy classes: RowPolicy, ColumnPolicy, BoxPolicy, LeafPolicy
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

    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override {
        int n = children.size();
        if (n == 0) {
            return {constraints.minWidth, constraints.minHeight};
        }

        std::vector<Placeable> placeables;
        placeables.reserve(n);
        std::vector<int> widths;
        widths.reserve(n);

        int totalSpacing = config_.spacing * (n - 1);
        int remainingWidth = constraints.hasBoundedWidth()
                                 ? constraints.maxWidth - totalSpacing
                                 : Infinity;

        for (auto* child : children) {
            int maxW = std::max(0, remainingWidth);
            Constraints childConstraints = {0, maxW, constraints.minHeight, constraints.maxHeight};
            auto placeable = child->measure(childConstraints);
            placeables.push_back(placeable);
            widths.push_back(placeable.width());
            if (remainingWidth != Infinity) {
                remainingWidth -= placeable.width();
            }
        }

        int totalWidth = std::accumulate(widths.begin(), widths.end(), 0) + totalSpacing;
        int maxHeight = 0;
        for (auto& p : placeables) {
            maxHeight = std::max(maxHeight, p.height());
        }
        int layoutWidth = constraints.constrainWidth(totalWidth);
        int layoutHeight = constraints.constrainHeight(maxHeight);

        auto positions = arrange(config_.arrangement, layoutWidth, widths, config_.spacing);
        for (int i = 0; i < n; i++) {
            int crossOffset = align(config_.crossAxisAlignment, placeables[i].height(), layoutHeight);
            placeables[i].placeAt(positions[i], crossOffset);
        }

        return {layoutWidth, layoutHeight};
    }
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

    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override {
        int n = children.size();
        if (n == 0) {
            return {constraints.minWidth, constraints.minHeight};
        }

        std::vector<Placeable> placeables;
        placeables.reserve(n);
        std::vector<int> heights;
        heights.reserve(n);

        int totalSpacing = config_.spacing * (n - 1);
        int remainingHeight = constraints.hasBoundedHeight()
                                  ? constraints.maxHeight - totalSpacing
                                  : Infinity;

        for (auto* child : children) {
            int maxH = std::max(0, remainingHeight);
            Constraints childConstraints = {constraints.minWidth, constraints.maxWidth, 0, maxH};
            auto placeable = child->measure(childConstraints);
            placeables.push_back(placeable);
            heights.push_back(placeable.height());
            if (remainingHeight != Infinity) {
                remainingHeight -= placeable.height();
            }
        }

        int totalHeight = std::accumulate(heights.begin(), heights.end(), 0) + totalSpacing;
        int maxWidth = 0;
        for (auto& p : placeables) {
            maxWidth = std::max(maxWidth, p.width());
        }
        int layoutWidth = constraints.constrainWidth(maxWidth);
        int layoutHeight = constraints.constrainHeight(totalHeight);

        auto positions = arrange(config_.arrangement, layoutHeight, heights, config_.spacing);
        for (int i = 0; i < n; i++) {
            int crossOffset = align(config_.crossAxisAlignment, placeables[i].width(), layoutWidth);
            placeables[i].placeAt(crossOffset, positions[i]);
        }

        return {layoutWidth, layoutHeight};
    }
};

struct BoxConfig {
    Alignment horizontalAlignment = Alignment::Start;
    Alignment verticalAlignment = Alignment::Start;
};

class BoxPolicy : public MeasurePolicy {
    BoxConfig config_;

public:
    explicit BoxPolicy(BoxConfig config = {}) : config_(config) {}

    MeasureResult Measure(LayoutNodeVec children, Constraints constraints) override {
        if (children.empty()) {
            return {constraints.minWidth, constraints.minHeight};
        }

        std::vector<Placeable> placeables;
        placeables.reserve(children.size());
        Constraints childConstraints = constraints.loosen();

        int maxWidth = 0;
        int maxHeight = 0;
        for (auto* child : children) {
            auto placeable = child->measure(childConstraints);
            placeables.push_back(placeable);
            maxWidth = std::max(maxWidth, placeable.width());
            maxHeight = std::max(maxHeight, placeable.height());
        }

        int layoutWidth = constraints.constrainWidth(maxWidth);
        int layoutHeight = constraints.constrainHeight(maxHeight);

        for (auto& p : placeables) {
            int x = align(config_.horizontalAlignment, p.width(), layoutWidth);
            int y = align(config_.verticalAlignment, p.height(), layoutHeight);
            p.placeAt(x, y);
        }

        return {layoutWidth, layoutHeight};
    }
};

// ============================================================================
// Leaf node - a fixed-size terminal node
// ============================================================================

struct LeafConfig {
    int width = 0;
    int height = 0;
};

class LeafPolicy : public MeasurePolicy {
    LeafConfig config_;

public:
    explicit LeafPolicy(LeafConfig config) : config_(config) {}

    MeasureResult Measure(LayoutNodeVec /*children*/, Constraints constraints) override {
        return {constraints.constrainWidth(config_.width),
                constraints.constrainHeight(config_.height)};
    }
};

// ============================================================================
// Builder functions - convenient node construction
// ============================================================================

inline LayoutNode* Leaf(LeafConfig config) {
    return new LayoutNode(new LeafPolicy(config));
}

inline LayoutNode* Row(RowConfig config, LayoutNodeVec children) {
    return new LayoutNode(new RowPolicy(config), children);
}

inline LayoutNode* Column(ColumnConfig config, LayoutNodeVec children) {
    return new LayoutNode(new ColumnPolicy(config), children);
}

inline LayoutNode* Box(BoxConfig config, LayoutNodeVec children) {
    return new LayoutNode(new BoxPolicy(config), children);
}

inline LayoutNode* Row(LayoutNodeVec children) {
    return Row({}, children);
}

inline LayoutNode* Column(LayoutNodeVec children) {
    return Column({}, children);
}

inline LayoutNode* Box(LayoutNodeVec children) {
    return Box({}, children);
}

inline LayoutNode* Layout(MeasurePolicy* policy, LayoutNodeVec children = {}) {
    return new LayoutNode(policy, children);
}

} // namespace compose
