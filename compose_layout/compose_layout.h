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

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
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

    // Create constraints for a child in a horizontal layout (Row):
    // the child can take up to remainingWidth, full height range preserved.
    constexpr Constraints withMaxWidth(int w) const {
        return {std::min(minWidth, w), w, minHeight, maxHeight};
    }

    constexpr Constraints withMaxHeight(int h) const {
        return {minWidth, maxWidth, std::min(minHeight, h), h};
    }

    // Remove minimum constraints (allow child to be smaller)
    constexpr Constraints loosen() const {
        return {0, maxWidth, 0, maxHeight};
    }
};

// ============================================================================
// LayoutNode - the core tree node
// ============================================================================

class LayoutNode;

// Placeable: a non-owning handle to a measured LayoutNode.
// Returned by LayoutNode::measure(), used to query size and place the child.
class Placeable {
    LayoutNode* node_;

public:
    explicit Placeable(LayoutNode* node) : node_(node) {}

    int width() const;
    int height() const;

    void placeAt(int x, int y);

    LayoutNode* node() const { return node_; }
};

// MeasurePolicy: given a list of child Placeables (already measured) and the
// original constraints, returns the layout's own size. Placement is done
// by calling placeAt() on each Placeable inside the policy.
//
// The two-phase approach:
//   Phase 1 (measure): Policy measures children by calling child.measure(constraints).
//   Phase 2 (place): After parent size is known, policy places children.
//
// We combine both phases: the policy receives Measurables (LayoutNode pointers),
// constraints, and returns {width, height}. It measures children (getting Placeables)
// and stores placement info. Placement is applied when place() is called on the parent.
struct MeasureResult {
    int width = 0;
    int height = 0;
};

// A MeasurePolicy receives the children and constraints, measures children
// (calling child->measure()), places them (calling placeable.placeAt()), and
// returns the layout's own size.
using MeasurePolicy = std::function<MeasureResult(
    std::span<std::unique_ptr<LayoutNode>> children,
    Constraints constraints)>;

class LayoutNode {
    MeasurePolicy policy_;
    std::vector<std::unique_ptr<LayoutNode>> children_;

    // Measurement state
    int measuredWidth_ = 0;
    int measuredHeight_ = 0;
    int x_ = 0;
    int y_ = 0;
    bool measured_ = false;

public:
    explicit LayoutNode(MeasurePolicy policy,
                        std::vector<std::unique_ptr<LayoutNode>> children = {})
        : policy_(std::move(policy)), children_(std::move(children)) {}

    // Measure this node with the given constraints.
    // Returns a Placeable handle for the parent to query size and place.
    Placeable measure(Constraints constraints) {
        if (policy_) {
            auto result = policy_(std::span(children_), constraints);
            measuredWidth_ = constraints.constrainWidth(result.width);
            measuredHeight_ = constraints.constrainHeight(result.height);
        }
        measured_ = true;
        return Placeable(this);
    }

    // Place this node at (x, y) relative to its parent.
    // Recursively offsets children's absolute positions.
    void place(int x, int y) {
        x_ = x;
        y_ = y;
    }

    // Accessors
    int measuredWidth() const { return measuredWidth_; }
    int measuredHeight() const { return measuredHeight_; }
    int x() const { return x_; }
    int y() const { return y_; }
    bool isMeasured() const { return measured_; }

    std::span<const std::unique_ptr<LayoutNode>> children() const {
        return children_;
    }

    // Get absolute position by walking up (for debugging/rendering).
    // Since we store positions relative to parent, absolute = sum of ancestors.
    // For simplicity, each node stores its position relative to parent.

    // Debug: collect all leaf bounds as {x, y, w, h} in absolute coordinates
    struct Rect {
        int x, y, width, height;
    };

    void collectBounds(std::vector<Rect>& out, int offsetX = 0, int offsetY = 0) const {
        int absX = offsetX + x_;
        int absY = offsetY + y_;
        if (children_.empty()) {
            out.push_back({absX, absY, measuredWidth_, measuredHeight_});
        }
        for (auto& child : children_) {
            child->collectBounds(out, absX, absY);
        }
    }
};

// Implement Placeable methods (need full LayoutNode definition)
inline int Placeable::width() const { return node_->measuredWidth(); }
inline int Placeable::height() const { return node_->measuredHeight(); }
inline void Placeable::placeAt(int x, int y) { node_->place(x, y); }

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

// Given total available space, child sizes, and spacing, compute positions.
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
// Built-in MeasurePolicy factories: Row, Column, Box
// ============================================================================

struct RowConfig {
    int spacing = 0;
    Arrangement arrangement = Arrangement::Start;
    Alignment crossAxisAlignment = Alignment::Start;
};

inline MeasurePolicy rowPolicy(RowConfig config = {}) {
    return [config](std::span<std::unique_ptr<LayoutNode>> children,
                    Constraints constraints) -> MeasureResult {
        int n = static_cast<int>(children.size());
        if (n == 0) {
            return {constraints.minWidth, constraints.minHeight};
        }

        // Phase 1: Measure children
        std::vector<Placeable> placeables;
        placeables.reserve(n);
        std::vector<int> widths;
        widths.reserve(n);

        int totalSpacing = config.spacing * (n - 1);
        int remainingWidth = constraints.hasBoundedWidth()
                                 ? constraints.maxWidth - totalSpacing
                                 : Infinity;

        for (auto& child : children) {
            int maxW = std::max(0, remainingWidth);
            Constraints childConstraints = {
                0, maxW, constraints.minHeight, constraints.maxHeight};
            auto placeable = child->measure(childConstraints);
            placeables.push_back(placeable);
            widths.push_back(placeable.width());
            if (remainingWidth != Infinity) {
                remainingWidth -= placeable.width();
            }
        }

        // Determine own size
        int totalWidth = std::accumulate(widths.begin(), widths.end(), 0) + totalSpacing;
        int maxHeight = 0;
        for (auto& p : placeables) {
            maxHeight = std::max(maxHeight, p.height());
        }
        int layoutWidth = constraints.constrainWidth(totalWidth);
        int layoutHeight = constraints.constrainHeight(maxHeight);

        // Phase 2: Place children
        auto positions = arrange(config.arrangement, layoutWidth, widths, config.spacing);
        for (int i = 0; i < n; i++) {
            int crossOffset = align(config.crossAxisAlignment, placeables[i].height(), layoutHeight);
            placeables[i].placeAt(positions[i], crossOffset);
        }

        return {layoutWidth, layoutHeight};
    };
}

struct ColumnConfig {
    int spacing = 0;
    Arrangement arrangement = Arrangement::Start;
    Alignment crossAxisAlignment = Alignment::Start;
};

inline MeasurePolicy columnPolicy(ColumnConfig config = {}) {
    return [config](std::span<std::unique_ptr<LayoutNode>> children,
                    Constraints constraints) -> MeasureResult {
        int n = static_cast<int>(children.size());
        if (n == 0) {
            return {constraints.minWidth, constraints.minHeight};
        }

        // Phase 1: Measure children
        std::vector<Placeable> placeables;
        placeables.reserve(n);
        std::vector<int> heights;
        heights.reserve(n);

        int totalSpacing = config.spacing * (n - 1);
        int remainingHeight = constraints.hasBoundedHeight()
                                  ? constraints.maxHeight - totalSpacing
                                  : Infinity;

        for (auto& child : children) {
            int maxH = std::max(0, remainingHeight);
            Constraints childConstraints = {
                constraints.minWidth, constraints.maxWidth, 0, maxH};
            auto placeable = child->measure(childConstraints);
            placeables.push_back(placeable);
            heights.push_back(placeable.height());
            if (remainingHeight != Infinity) {
                remainingHeight -= placeable.height();
            }
        }

        // Determine own size
        int totalHeight = std::accumulate(heights.begin(), heights.end(), 0) + totalSpacing;
        int maxWidth = 0;
        for (auto& p : placeables) {
            maxWidth = std::max(maxWidth, p.width());
        }
        int layoutWidth = constraints.constrainWidth(maxWidth);
        int layoutHeight = constraints.constrainHeight(totalHeight);

        // Phase 2: Place children
        auto positions = arrange(config.arrangement, layoutHeight, heights, config.spacing);
        for (int i = 0; i < n; i++) {
            int crossOffset = align(config.crossAxisAlignment, placeables[i].width(), layoutWidth);
            placeables[i].placeAt(crossOffset, positions[i]);
        }

        return {layoutWidth, layoutHeight};
    };
}

struct BoxConfig {
    Alignment horizontalAlignment = Alignment::Start;
    Alignment verticalAlignment = Alignment::Start;
};

inline MeasurePolicy boxPolicy(BoxConfig config = {}) {
    return [config](std::span<std::unique_ptr<LayoutNode>> children,
                    Constraints constraints) -> MeasureResult {
        if (children.empty()) {
            return {constraints.minWidth, constraints.minHeight};
        }

        // Phase 1: Measure all children with loosened constraints
        std::vector<Placeable> placeables;
        placeables.reserve(children.size());
        Constraints childConstraints = constraints.loosen();

        int maxWidth = 0;
        int maxHeight = 0;
        for (auto& child : children) {
            auto placeable = child->measure(childConstraints);
            placeables.push_back(placeable);
            maxWidth = std::max(maxWidth, placeable.width());
            maxHeight = std::max(maxHeight, placeable.height());
        }

        int layoutWidth = constraints.constrainWidth(maxWidth);
        int layoutHeight = constraints.constrainHeight(maxHeight);

        // Phase 2: Place children (stacked, aligned)
        for (auto& p : placeables) {
            int x = align(config.horizontalAlignment, p.width(), layoutWidth);
            int y = align(config.verticalAlignment, p.height(), layoutHeight);
            p.placeAt(x, y);
        }

        return {layoutWidth, layoutHeight};
    };
}

// ============================================================================
// Leaf node - a fixed-size or intrinsic-size terminal node
// ============================================================================

struct LeafConfig {
    int width = 0;
    int height = 0;
};

inline MeasurePolicy leafPolicy(LeafConfig config) {
    return [config](std::span<std::unique_ptr<LayoutNode>> /*children*/,
                    Constraints constraints) -> MeasureResult {
        return {constraints.constrainWidth(config.width),
                constraints.constrainHeight(config.height)};
    };
}

// ============================================================================
// Builder functions - convenient node construction
//
// Uses variadic templates because std::initializer_list doesn't support
// move-only types (unique_ptr). The fold expression packs children into a vector.
// ============================================================================

namespace detail {

inline void packChildren(std::vector<std::unique_ptr<LayoutNode>>& /*vec*/) {}

template <typename... Rest>
inline void packChildren(std::vector<std::unique_ptr<LayoutNode>>& vec,
                          std::unique_ptr<LayoutNode> first, Rest&&... rest) {
    vec.push_back(std::move(first));
    packChildren(vec, std::forward<Rest>(rest)...);
}

template <typename... Children>
inline std::vector<std::unique_ptr<LayoutNode>> makeChildren(Children&&... children) {
    std::vector<std::unique_ptr<LayoutNode>> vec;
    vec.reserve(sizeof...(children));
    packChildren(vec, std::forward<Children>(children)...);
    return vec;
}

} // namespace detail

inline std::unique_ptr<LayoutNode> Leaf(LeafConfig config) {
    return std::make_unique<LayoutNode>(leafPolicy(config));
}

template <typename... Children>
inline std::unique_ptr<LayoutNode> Row(RowConfig config, Children&&... children) {
    return std::make_unique<LayoutNode>(
        rowPolicy(config), detail::makeChildren(std::forward<Children>(children)...));
}

template <typename... Children>
inline std::unique_ptr<LayoutNode> Column(ColumnConfig config, Children&&... children) {
    return std::make_unique<LayoutNode>(
        columnPolicy(config), detail::makeChildren(std::forward<Children>(children)...));
}

template <typename... Children>
inline std::unique_ptr<LayoutNode> Box(BoxConfig config, Children&&... children) {
    return std::make_unique<LayoutNode>(
        boxPolicy(config), detail::makeChildren(std::forward<Children>(children)...));
}

// Overloads with default config (need at least one child to disambiguate from config version)
template <typename First, typename... Rest>
    requires std::convertible_to<First, std::unique_ptr<LayoutNode>>
inline std::unique_ptr<LayoutNode> Row(First&& first, Rest&&... rest) {
    return Row(RowConfig{}, std::forward<First>(first), std::forward<Rest>(rest)...);
}

template <typename First, typename... Rest>
    requires std::convertible_to<First, std::unique_ptr<LayoutNode>>
inline std::unique_ptr<LayoutNode> Column(First&& first, Rest&&... rest) {
    return Row(ColumnConfig{}, std::forward<First>(first), std::forward<Rest>(rest)...);
}

template <typename First, typename... Rest>
    requires std::convertible_to<First, std::unique_ptr<LayoutNode>>
inline std::unique_ptr<LayoutNode> Box(First&& first, Rest&&... rest) {
    return Box(BoxConfig{}, std::forward<First>(first), std::forward<Rest>(rest)...);
}

// Custom layout node with a user-provided MeasurePolicy
template <typename... Children>
inline std::unique_ptr<LayoutNode> Layout(MeasurePolicy policy, Children&&... children) {
    return std::make_unique<LayoutNode>(
        std::move(policy), detail::makeChildren(std::forward<Children>(children)...));
}

} // namespace compose
