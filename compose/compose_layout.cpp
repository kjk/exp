#include "compose_layout.h"

#include <numeric>

namespace compose {

// ============================================================================
// LayoutNodeVec
// ============================================================================

LayoutNodeVec::LayoutNodeVec(std::initializer_list<LayoutNode*> list)
    : len(static_cast<int>(list.size())),
      els(list.size() ? new LayoutNode*[list.size()] : nullptr) {
    int i = 0;
    for (auto* node : list) {
        els[i++] = node;
    }
}

void freeVec(LayoutNodeVec& vec) {
    delete[] vec.els;
    vec.els = nullptr;
    vec.len = 0;
}

// ============================================================================
// Placeable
// ============================================================================

int Placeable::width() const { return node_->measuredWidth(); }
int Placeable::height() const { return node_->measuredHeight(); }
void Placeable::placeAt(int x, int y) { node_->place(x, y); }

// ============================================================================
// LayoutNode
// ============================================================================

LayoutNode::LayoutNode(MeasurePolicy* policy, LayoutNodeVec children)
    : policy_(policy), children_(children) {}

LayoutNode::~LayoutNode() {
    delete policy_;
    delete[] children_.els;
}

Placeable LayoutNode::measure(Constraints constraints) {
    if (policy_) {
        auto result = policy_->Measure(children_, constraints);
        measuredWidth_ = constraints.constrainWidth(result.width);
        measuredHeight_ = constraints.constrainHeight(result.height);
    }
    measured_ = true;
    return Placeable(this);
}

void LayoutNode::collectBounds(std::vector<Rect>& out, int offsetX, int offsetY) const {
    int absX = offsetX + x_;
    int absY = offsetY + y_;
    if (children_.empty()) {
        out.push_back({absX, absY, measuredWidth_, measuredHeight_});
    }
    for (auto* child : children_) {
        child->collectBounds(out, absX, absY);
    }
}

void freeTree(LayoutNode* node) {
    if (!node) return;
    for (auto* child : node->children()) {
        freeTree(child);
    }
    delete node;
}

// ============================================================================
// Arrangement & Alignment
// ============================================================================

std::vector<int> arrange(Arrangement arrangement, int totalSize,
                         std::span<const int> childSizes, int spacing) {
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

int align(Alignment alignment, int childSize, int parentSize) {
    switch (alignment) {
    case Alignment::Start: return 0;
    case Alignment::Center: return (parentSize - childSize) / 2;
    case Alignment::End: return parentSize - childSize;
    }
    return 0;
}

// ============================================================================
// RowPolicy
// ============================================================================

MeasureResult RowPolicy::Measure(LayoutNodeVec children, Constraints constraints) {
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

// ============================================================================
// ColumnPolicy
// ============================================================================

MeasureResult ColumnPolicy::Measure(LayoutNodeVec children, Constraints constraints) {
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

// ============================================================================
// BoxPolicy
// ============================================================================

// BoxPolicy stacks all children on top of each other (like FrameLayout in Android).
// 1. Measures each child with loosened constraints (no minimum, preserves max bounds),
//    so children are free to be any size up to the parent's max.
// 2. The box's own size is the max width and height across all children,
//    clamped to the incoming constraints.
// 3. Each child is aligned within the box according to horizontalAlignment
//    and verticalAlignment (Start, Center, or End on each axis).
MeasureResult BoxPolicy::Measure(LayoutNodeVec children, Constraints constraints) {
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

// ============================================================================
// LeafPolicy
// ============================================================================

MeasureResult LeafPolicy::Measure(LayoutNodeVec /*children*/, Constraints constraints) {
    return {constraints.constrainWidth(config_.width),
            constraints.constrainHeight(config_.height)};
}

// ============================================================================
// Builder functions
// ============================================================================

LayoutNode* Leaf(LeafConfig config) {
    return new LayoutNode(new LeafPolicy(config));
}

LayoutNode* Row(RowConfig config, LayoutNodeVec children) {
    return new LayoutNode(new RowPolicy(config), children);
}

LayoutNode* Column(ColumnConfig config, LayoutNodeVec children) {
    return new LayoutNode(new ColumnPolicy(config), children);
}

LayoutNode* Box(BoxConfig config, LayoutNodeVec children) {
    return new LayoutNode(new BoxPolicy(config), children);
}

LayoutNode* Row(LayoutNodeVec children) {
    return Row({}, children);
}

LayoutNode* Column(LayoutNodeVec children) {
    return Column({}, children);
}

LayoutNode* Box(LayoutNodeVec children) {
    return Box({}, children);
}

LayoutNode* Layout(MeasurePolicy* policy, LayoutNodeVec children) {
    return new LayoutNode(policy, children);
}

} // namespace compose
