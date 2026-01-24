#include "layout.h"

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

// ============================================================================
// MeasurePolicy default intrinsics
// ============================================================================

int MeasurePolicy::MinIntrinsicWidth(LayoutNodeVec children, int height) {
    // Default: measure with height constraint and return resulting width.
    int maxW = 0;
    for (auto* child : children) {
        maxW = std::max(maxW, child->minIntrinsicWidth(height));
    }
    return maxW;
}

int MeasurePolicy::MaxIntrinsicWidth(LayoutNodeVec children, int height) {
    int maxW = 0;
    for (auto* child : children) {
        maxW = std::max(maxW, child->maxIntrinsicWidth(height));
    }
    return maxW;
}

int MeasurePolicy::MinIntrinsicHeight(LayoutNodeVec children, int width) {
    int maxH = 0;
    for (auto* child : children) {
        maxH = std::max(maxH, child->minIntrinsicHeight(width));
    }
    return maxH;
}

int MeasurePolicy::MaxIntrinsicHeight(LayoutNodeVec children, int width) {
    int maxH = 0;
    for (auto* child : children) {
        maxH = std::max(maxH, child->maxIntrinsicHeight(width));
    }
    return maxH;
}

// ============================================================================
// LayoutNode
// ============================================================================

Placeable LayoutNode::measure(Constraints constraints) {
    if (policy_) {
        auto result = policy_->Measure(children_, constraints);
        measuredWidth_ = constraints.constrainWidth(result.width);
        measuredHeight_ = constraints.constrainHeight(result.height);
    }
    measured_ = true;
    return Placeable(this);
}

int LayoutNode::minIntrinsicWidth(int height) {
    if (policy_) return policy_->MinIntrinsicWidth(children_, height);
    return 0;
}

int LayoutNode::maxIntrinsicWidth(int height) {
    if (policy_) return policy_->MaxIntrinsicWidth(children_, height);
    return 0;
}

int LayoutNode::minIntrinsicHeight(int width) {
    if (policy_) return policy_->MinIntrinsicHeight(children_, width);
    return 0;
}

int LayoutNode::maxIntrinsicHeight(int width) {
    if (policy_) return policy_->MaxIntrinsicHeight(children_, width);
    return 0;
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

    std::vector<Placeable> placeables(n, Placeable(nullptr));
    std::vector<int> widths(n, 0);

    int totalSpacing = config_.spacing * (n - 1);
    int remainingWidth = constraints.hasBoundedWidth()
                             ? constraints.maxWidth - totalSpacing
                             : Infinity;

    // Phase 1: Measure non-weighted children first.
    float totalWeight = 0.0f;
    for (int i = 0; i < n; i++) {
        float w = children[i]->weight();
        if (w > 0.0f) {
            totalWeight += w;
            continue;
        }
        int maxW = std::max(0, remainingWidth);
        Constraints childConstraints = {0, maxW, constraints.minHeight, constraints.maxHeight};
        placeables[i] = children[i]->measure(childConstraints);
        widths[i] = placeables[i].width();
        if (remainingWidth != Infinity) {
            remainingWidth -= placeables[i].width();
        }
    }

    // Phase 2: Distribute remaining space among weighted children.
    if (totalWeight > 0.0f) {
        int spaceForWeighted = std::max(0, remainingWidth);
        for (int i = 0; i < n; i++) {
            float w = children[i]->weight();
            if (w <= 0.0f) continue;
            int allocation = static_cast<int>(spaceForWeighted * (w / totalWeight));
            Constraints childConstraints;
            if (children[i]->fillWeight()) {
                childConstraints = {allocation, allocation, constraints.minHeight, constraints.maxHeight};
            } else {
                childConstraints = {0, allocation, constraints.minHeight, constraints.maxHeight};
            }
            placeables[i] = children[i]->measure(childConstraints);
            widths[i] = placeables[i].width();
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

int RowPolicy::MinIntrinsicWidth(LayoutNodeVec children, int height) {
    int n = children.size();
    if (n == 0) return 0;
    int total = config_.spacing * (n - 1);
    for (auto* child : children) {
        total += child->minIntrinsicWidth(height);
    }
    return total;
}

int RowPolicy::MaxIntrinsicWidth(LayoutNodeVec children, int height) {
    int n = children.size();
    if (n == 0) return 0;
    int total = config_.spacing * (n - 1);
    for (auto* child : children) {
        total += child->maxIntrinsicWidth(height);
    }
    return total;
}

int RowPolicy::MinIntrinsicHeight(LayoutNodeVec children, int /*width*/) {
    int maxH = 0;
    for (auto* child : children) {
        maxH = std::max(maxH, child->minIntrinsicHeight(Infinity));
    }
    return maxH;
}

int RowPolicy::MaxIntrinsicHeight(LayoutNodeVec children, int /*width*/) {
    int maxH = 0;
    for (auto* child : children) {
        maxH = std::max(maxH, child->maxIntrinsicHeight(Infinity));
    }
    return maxH;
}

// ============================================================================
// ColumnPolicy
// ============================================================================

MeasureResult ColumnPolicy::Measure(LayoutNodeVec children, Constraints constraints) {
    int n = children.size();
    if (n == 0) {
        return {constraints.minWidth, constraints.minHeight};
    }

    std::vector<Placeable> placeables(n, Placeable(nullptr));
    std::vector<int> heights(n, 0);

    int totalSpacing = config_.spacing * (n - 1);
    int remainingHeight = constraints.hasBoundedHeight()
                              ? constraints.maxHeight - totalSpacing
                              : Infinity;

    // Phase 1: Measure non-weighted children first.
    float totalWeight = 0.0f;
    for (int i = 0; i < n; i++) {
        float w = children[i]->weight();
        if (w > 0.0f) {
            totalWeight += w;
            continue;
        }
        int maxH = std::max(0, remainingHeight);
        Constraints childConstraints = {constraints.minWidth, constraints.maxWidth, 0, maxH};
        placeables[i] = children[i]->measure(childConstraints);
        heights[i] = placeables[i].height();
        if (remainingHeight != Infinity) {
            remainingHeight -= placeables[i].height();
        }
    }

    // Phase 2: Distribute remaining space among weighted children.
    if (totalWeight > 0.0f) {
        int spaceForWeighted = std::max(0, remainingHeight);
        for (int i = 0; i < n; i++) {
            float w = children[i]->weight();
            if (w <= 0.0f) continue;
            int allocation = static_cast<int>(spaceForWeighted * (w / totalWeight));
            Constraints childConstraints;
            if (children[i]->fillWeight()) {
                childConstraints = {constraints.minWidth, constraints.maxWidth, allocation, allocation};
            } else {
                childConstraints = {constraints.minWidth, constraints.maxWidth, 0, allocation};
            }
            placeables[i] = children[i]->measure(childConstraints);
            heights[i] = placeables[i].height();
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

int ColumnPolicy::MinIntrinsicWidth(LayoutNodeVec children, int /*height*/) {
    int maxW = 0;
    for (auto* child : children) {
        maxW = std::max(maxW, child->minIntrinsicWidth(Infinity));
    }
    return maxW;
}

int ColumnPolicy::MaxIntrinsicWidth(LayoutNodeVec children, int /*height*/) {
    int maxW = 0;
    for (auto* child : children) {
        maxW = std::max(maxW, child->maxIntrinsicWidth(Infinity));
    }
    return maxW;
}

int ColumnPolicy::MinIntrinsicHeight(LayoutNodeVec children, int width) {
    int n = children.size();
    if (n == 0) return 0;
    int total = config_.spacing * (n - 1);
    for (auto* child : children) {
        total += child->minIntrinsicHeight(width);
    }
    return total;
}

int ColumnPolicy::MaxIntrinsicHeight(LayoutNodeVec children, int width) {
    int n = children.size();
    if (n == 0) return 0;
    int total = config_.spacing * (n - 1);
    for (auto* child : children) {
        total += child->maxIntrinsicHeight(width);
    }
    return total;
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
    int n = children.size();
    if (n == 0) {
        return {constraints.minWidth, constraints.minHeight};
    }

    std::vector<Placeable> placeables(n, Placeable(nullptr));
    Constraints childConstraints = config_.propagateMinConstraints
        ? constraints
        : constraints.loosen();

    // Phase 1: Measure children that do NOT have matchParentSize.
    // These determine the Box's own size.
    int maxWidth = 0;
    int maxHeight = 0;
    bool hasMatchParent = false;
    for (int i = 0; i < n; i++) {
        if (children[i]->matchParentSize()) {
            hasMatchParent = true;
            continue;
        }
        placeables[i] = Placeable(children[i]);
        auto placeable = children[i]->measure(childConstraints);
        placeables[i] = placeable;
        maxWidth = std::max(maxWidth, placeable.width());
        maxHeight = std::max(maxHeight, placeable.height());
    }

    int layoutWidth = constraints.constrainWidth(maxWidth);
    int layoutHeight = constraints.constrainHeight(maxHeight);

    // Phase 2: Measure matchParentSize children with exact constraints
    // equal to the resolved Box size.
    if (hasMatchParent) {
        Constraints matchConstraints = {layoutWidth, layoutWidth, layoutHeight, layoutHeight};
        for (int i = 0; i < n; i++) {
            if (!children[i]->matchParentSize()) continue;
            placeables[i] = children[i]->measure(matchConstraints);
        }
    }

    for (int i = 0; i < n; i++) {
        int x = align(config_.horizontalAlignment, placeables[i].width(), layoutWidth);
        int y = align(config_.verticalAlignment, placeables[i].height(), layoutHeight);
        placeables[i].placeAt(x, y);
    }

    return {layoutWidth, layoutHeight};
}

int BoxPolicy::MinIntrinsicWidth(LayoutNodeVec children, int height) {
    int maxW = 0;
    for (auto* child : children) {
        maxW = std::max(maxW, child->minIntrinsicWidth(height));
    }
    return maxW;
}

int BoxPolicy::MaxIntrinsicWidth(LayoutNodeVec children, int height) {
    int maxW = 0;
    for (auto* child : children) {
        maxW = std::max(maxW, child->maxIntrinsicWidth(height));
    }
    return maxW;
}

int BoxPolicy::MinIntrinsicHeight(LayoutNodeVec children, int width) {
    int maxH = 0;
    for (auto* child : children) {
        maxH = std::max(maxH, child->minIntrinsicHeight(width));
    }
    return maxH;
}

int BoxPolicy::MaxIntrinsicHeight(LayoutNodeVec children, int width) {
    int maxH = 0;
    for (auto* child : children) {
        maxH = std::max(maxH, child->maxIntrinsicHeight(width));
    }
    return maxH;
}

// ============================================================================
// LeafPolicy
// ============================================================================

MeasureResult LeafPolicy::Measure(LayoutNodeVec /*children*/, Constraints constraints) {
    return {constraints.constrainWidth(config_.width),
            constraints.constrainHeight(config_.height)};
}

int LeafPolicy::MinIntrinsicWidth(LayoutNodeVec /*children*/, int /*height*/) {
    return config_.width;
}

int LeafPolicy::MaxIntrinsicWidth(LayoutNodeVec /*children*/, int /*height*/) {
    return config_.width;
}

int LeafPolicy::MinIntrinsicHeight(LayoutNodeVec /*children*/, int /*width*/) {
    return config_.height;
}

int LeafPolicy::MaxIntrinsicHeight(LayoutNodeVec /*children*/, int /*width*/) {
    return config_.height;
}

// ============================================================================
// FlowRowPolicy
// ============================================================================

// FlowRow lays out children horizontally, wrapping to the next line when a child
// would overflow the available width. Each line is independently arranged, and
// lines are stacked vertically with crossAxisSpacing between them.
// Supports maxLines, overflow handling, and per-line weight distribution.
MeasureResult FlowRowPolicy::Measure(LayoutNodeVec children, Constraints constraints) {
    int n = children.size();
    if (n == 0) {
        return {constraints.minWidth, constraints.minHeight};
    }

    int maxWidth = constraints.hasBoundedWidth() ? constraints.maxWidth : Infinity;

    // Phase 1: Measure non-weighted children with loosened constraints.
    // Weighted children are deferred to per-line distribution.
    std::vector<Placeable> placeables(n, Placeable(nullptr));
    std::vector<int> measuredWidths(n, 0);
    for (int i = 0; i < n; i++) {
        if (children[i]->weight() > 0.0f) continue;
        Constraints childConstraints = {0, maxWidth, 0, constraints.maxHeight};
        placeables[i] = children[i]->measure(childConstraints);
        measuredWidths[i] = placeables[i].width();
    }

    // Phase 2: Break children into lines based on available width.
    struct Line {
        int startIdx;
        int count;
        int width;   // total width of non-weighted items + spacing
        int height;  // tallest item in this line
    };
    std::vector<Line> lines;
    int lineStart = 0;
    int lineWidth = 0;
    int lineHeight = 0;
    int lineCount = 0;

    for (int i = 0; i < n; i++) {
        int childWidth = measuredWidths[i];
        int spacingBefore = (lineCount > 0) ? config_.mainAxisSpacing : 0;
        int projectedWidth = lineWidth + spacingBefore + childWidth;

        bool overflow = (maxWidth != Infinity) && (projectedWidth > maxWidth) && (lineCount > 0);
        bool maxItems = (config_.maxItemsInEachRow > 0) && (lineCount >= config_.maxItemsInEachRow);

        if (overflow || maxItems) {
            lines.push_back({lineStart, lineCount, lineWidth, lineHeight});
            lineStart = i;
            lineWidth = childWidth;
            lineHeight = (children[i]->weight() > 0.0f) ? 0 : placeables[i].height();
            lineCount = 1;
        } else {
            lineWidth = projectedWidth;
            if (children[i]->weight() <= 0.0f) {
                lineHeight = std::max(lineHeight, placeables[i].height());
            }
            lineCount++;
        }
    }
    if (lineCount > 0) {
        lines.push_back({lineStart, lineCount, lineWidth, lineHeight});
    }

    // Apply maxLines limit.
    int visibleLines = static_cast<int>(lines.size());
    if (config_.maxLines > 0 && visibleLines > config_.maxLines) {
        if (config_.overflow == FlowOverflow::Clip) {
            visibleLines = config_.maxLines;
        }
    }

    // Phase 2b: Measure weighted children per-line with remaining space.
    for (int li = 0; li < visibleLines; li++) {
        auto& line = lines[li];
        float totalWeight = 0.0f;
        for (int i = 0; i < line.count; i++) {
            int idx = line.startIdx + i;
            float w = children[idx]->weight();
            if (w > 0.0f) totalWeight += w;
        }
        if (totalWeight <= 0.0f) continue;

        int lineSpacing = config_.mainAxisSpacing * (line.count - 1);
        int remaining = (maxWidth != Infinity)
            ? std::max(0, maxWidth - line.width - lineSpacing * (totalWeight > 0.0f ? 1 : 0))
            : 0;
        // Subtract spacing slots for weighted items not yet accounted for.
        // line.width already includes spacing for non-weighted items; we need to add
        // spacing for weighted items.
        int weightedCount = 0;
        for (int i = 0; i < line.count; i++) {
            int idx = line.startIdx + i;
            if (children[idx]->weight() > 0.0f) weightedCount++;
        }
        // Recalculate: total occupied = non-weighted widths + all spacing
        int nonWeightedWidth = 0;
        int nonWeightedCount = 0;
        for (int i = 0; i < line.count; i++) {
            int idx = line.startIdx + i;
            if (children[idx]->weight() <= 0.0f) {
                nonWeightedWidth += measuredWidths[idx];
                nonWeightedCount++;
            }
        }
        int allSpacing = config_.mainAxisSpacing * (line.count - 1);
        remaining = (maxWidth != Infinity)
            ? std::max(0, maxWidth - nonWeightedWidth - allSpacing)
            : 0;

        for (int i = 0; i < line.count; i++) {
            int idx = line.startIdx + i;
            float w = children[idx]->weight();
            if (w <= 0.0f) continue;
            int allocation = static_cast<int>(remaining * (w / totalWeight));
            Constraints wc;
            if (children[idx]->fillWeight()) {
                wc = {allocation, allocation, 0, constraints.maxHeight};
            } else {
                wc = {0, allocation, 0, constraints.maxHeight};
            }
            placeables[idx] = children[idx]->measure(wc);
            measuredWidths[idx] = placeables[idx].width();
            line.height = std::max(line.height, placeables[idx].height());
        }
        // Recalculate line width including weighted items.
        int newWidth = 0;
        for (int i = 0; i < line.count; i++) {
            int idx = line.startIdx + i;
            if (i > 0) newWidth += config_.mainAxisSpacing;
            newWidth += measuredWidths[idx];
        }
        line.width = newWidth;
    }

    // Phase 3: Compute layout size (only for visible lines).
    int layoutWidth = 0;
    for (int li = 0; li < visibleLines; li++) {
        layoutWidth = std::max(layoutWidth, lines[li].width);
    }
    layoutWidth = constraints.constrainWidth(layoutWidth);

    int totalHeight = 0;
    for (int li = 0; li < visibleLines; li++) {
        totalHeight += lines[li].height;
        if (li > 0) totalHeight += config_.crossAxisSpacing;
    }
    int layoutHeight = constraints.constrainHeight(totalHeight);

    // Phase 4: Place children line by line (only visible lines).
    int yOffset = 0;
    for (int li = 0; li < visibleLines; li++) {
        auto& line = lines[li];
        int xOffset = 0;
        for (int i = 0; i < line.count; i++) {
            int idx = line.startIdx + i;
            int crossOffset = align(config_.crossAxisAlignment, placeables[idx].height(), line.height);
            placeables[idx].placeAt(xOffset, yOffset + crossOffset);
            xOffset += measuredWidths[idx] + config_.mainAxisSpacing;
        }
        yOffset += line.height + config_.crossAxisSpacing;
    }

    return {layoutWidth, layoutHeight};
}

// ============================================================================
// FlowColumnPolicy
// ============================================================================

// FlowColumn lays out children vertically, wrapping to the next column when a child
// would overflow the available height. Each column is independently arranged, and
// columns are placed horizontally with crossAxisSpacing between them.
// Supports maxColumns, overflow handling, and per-column weight distribution.
MeasureResult FlowColumnPolicy::Measure(LayoutNodeVec children, Constraints constraints) {
    int n = children.size();
    if (n == 0) {
        return {constraints.minWidth, constraints.minHeight};
    }

    int maxHeight = constraints.hasBoundedHeight() ? constraints.maxHeight : Infinity;

    // Phase 1: Measure non-weighted children with loosened constraints.
    std::vector<Placeable> placeables(n, Placeable(nullptr));
    std::vector<int> measuredHeights(n, 0);
    for (int i = 0; i < n; i++) {
        if (children[i]->weight() > 0.0f) continue;
        Constraints childConstraints = {0, constraints.maxWidth, 0, maxHeight};
        placeables[i] = children[i]->measure(childConstraints);
        measuredHeights[i] = placeables[i].height();
    }

    // Phase 2: Break children into columns based on available height.
    struct Column {
        int startIdx;
        int count;
        int width;   // widest item in this column
        int height;  // total height of items + spacing
    };
    std::vector<Column> columns;
    int colStart = 0;
    int colHeight = 0;
    int colWidth = 0;
    int colCount = 0;

    for (int i = 0; i < n; i++) {
        int childHeight = measuredHeights[i];
        int spacingBefore = (colCount > 0) ? config_.mainAxisSpacing : 0;
        int projectedHeight = colHeight + spacingBefore + childHeight;

        bool overflow = (maxHeight != Infinity) && (projectedHeight > maxHeight) && (colCount > 0);
        bool maxItems = (config_.maxItemsInEachColumn > 0) && (colCount >= config_.maxItemsInEachColumn);

        if (overflow || maxItems) {
            columns.push_back({colStart, colCount, colWidth, colHeight});
            colStart = i;
            colHeight = childHeight;
            colWidth = (children[i]->weight() > 0.0f) ? 0 : placeables[i].width();
            colCount = 1;
        } else {
            colHeight = projectedHeight;
            if (children[i]->weight() <= 0.0f) {
                colWidth = std::max(colWidth, placeables[i].width());
            }
            colCount++;
        }
    }
    if (colCount > 0) {
        columns.push_back({colStart, colCount, colWidth, colHeight});
    }

    // Apply maxColumns limit.
    int visibleCols = static_cast<int>(columns.size());
    if (config_.maxColumns > 0 && visibleCols > config_.maxColumns) {
        if (config_.overflow == FlowOverflow::Clip) {
            visibleCols = config_.maxColumns;
        }
    }

    // Phase 2b: Measure weighted children per-column with remaining space.
    for (int ci = 0; ci < visibleCols; ci++) {
        auto& col = columns[ci];
        float totalWeight = 0.0f;
        for (int i = 0; i < col.count; i++) {
            int idx = col.startIdx + i;
            float w = children[idx]->weight();
            if (w > 0.0f) totalWeight += w;
        }
        if (totalWeight <= 0.0f) continue;

        int nonWeightedHeight = 0;
        for (int i = 0; i < col.count; i++) {
            int idx = col.startIdx + i;
            if (children[idx]->weight() <= 0.0f) {
                nonWeightedHeight += measuredHeights[idx];
            }
        }
        int allSpacing = config_.mainAxisSpacing * (col.count - 1);
        int remaining = (maxHeight != Infinity)
            ? std::max(0, maxHeight - nonWeightedHeight - allSpacing)
            : 0;

        for (int i = 0; i < col.count; i++) {
            int idx = col.startIdx + i;
            float w = children[idx]->weight();
            if (w <= 0.0f) continue;
            int allocation = static_cast<int>(remaining * (w / totalWeight));
            Constraints wc;
            if (children[idx]->fillWeight()) {
                wc = {0, constraints.maxWidth, allocation, allocation};
            } else {
                wc = {0, constraints.maxWidth, 0, allocation};
            }
            placeables[idx] = children[idx]->measure(wc);
            measuredHeights[idx] = placeables[idx].height();
            col.width = std::max(col.width, placeables[idx].width());
        }
        // Recalculate column height.
        int newHeight = 0;
        for (int i = 0; i < col.count; i++) {
            int idx = col.startIdx + i;
            if (i > 0) newHeight += config_.mainAxisSpacing;
            newHeight += measuredHeights[idx];
        }
        col.height = newHeight;
    }

    // Phase 3: Compute layout size (only for visible columns).
    int layoutHeight = 0;
    for (int ci = 0; ci < visibleCols; ci++) {
        layoutHeight = std::max(layoutHeight, columns[ci].height);
    }
    layoutHeight = constraints.constrainHeight(layoutHeight);

    int totalWidth = 0;
    for (int ci = 0; ci < visibleCols; ci++) {
        totalWidth += columns[ci].width;
        if (ci > 0) totalWidth += config_.crossAxisSpacing;
    }
    int layoutWidth = constraints.constrainWidth(totalWidth);

    // Phase 4: Place children column by column (only visible columns).
    int xOffset = 0;
    for (int ci = 0; ci < visibleCols; ci++) {
        auto& col = columns[ci];
        int yOffset = 0;
        for (int i = 0; i < col.count; i++) {
            int idx = col.startIdx + i;
            int crossOffset = align(config_.crossAxisAlignment, placeables[idx].width(), col.width);
            placeables[idx].placeAt(xOffset + crossOffset, yOffset);
            yOffset += measuredHeights[idx] + config_.mainAxisSpacing;
        }
        xOffset += col.width + config_.crossAxisSpacing;
    }

    return {layoutWidth, layoutHeight};
}

// ============================================================================
// FillMaxSizePolicy
// ============================================================================

MeasureResult FillMaxSizePolicy::Measure(LayoutNodeVec children, Constraints constraints) {
    Constraints childConstraints = constraints;
    if (config_.widthFraction >= 0.0f && constraints.hasBoundedWidth()) {
        int w = static_cast<int>(constraints.maxWidth * config_.widthFraction);
        childConstraints.minWidth = w;
        childConstraints.maxWidth = w;
    }
    if (config_.heightFraction >= 0.0f && constraints.hasBoundedHeight()) {
        int h = static_cast<int>(constraints.maxHeight * config_.heightFraction);
        childConstraints.minHeight = h;
        childConstraints.maxHeight = h;
    }
    if (!children.empty()) {
        auto placeable = children[0]->measure(childConstraints);
        placeable.placeAt(0, 0);
        return {placeable.width(), placeable.height()};
    }
    return {childConstraints.minWidth, childConstraints.minHeight};
}

// ============================================================================
// SizePolicy
// ============================================================================

MeasureResult SizePolicy::Measure(LayoutNodeVec children, Constraints constraints) {
    Constraints childConstraints = constraints;
    if (config_.width >= 0) {
        int w = constraints.constrainWidth(config_.width);
        childConstraints.minWidth = w;
        childConstraints.maxWidth = w;
    }
    if (config_.height >= 0) {
        int h = constraints.constrainHeight(config_.height);
        childConstraints.minHeight = h;
        childConstraints.maxHeight = h;
    }
    if (!children.empty()) {
        auto placeable = children[0]->measure(childConstraints);
        placeable.placeAt(0, 0);
        return {placeable.width(), placeable.height()};
    }
    return {childConstraints.minWidth, childConstraints.minHeight};
}

// ============================================================================
// RequiredSizePolicy
// ============================================================================

MeasureResult RequiredSizePolicy::Measure(LayoutNodeVec children, Constraints constraints) {
    Constraints childConstraints = constraints;
    if (config_.width >= 0) {
        childConstraints.minWidth = config_.width;
        childConstraints.maxWidth = config_.width;
    }
    if (config_.height >= 0) {
        childConstraints.minHeight = config_.height;
        childConstraints.maxHeight = config_.height;
    }
    if (!children.empty()) {
        auto placeable = children[0]->measure(childConstraints);
        // Center the child within the parent's allocated space.
        int parentW = constraints.constrainWidth(placeable.width());
        int parentH = constraints.constrainHeight(placeable.height());
        int x = (parentW - placeable.width()) / 2;
        int y = (parentH - placeable.height()) / 2;
        placeable.placeAt(x, y);
        return {parentW, parentH};
    }
    int w = config_.width >= 0 ? constraints.constrainWidth(config_.width) : constraints.minWidth;
    int h = config_.height >= 0 ? constraints.constrainHeight(config_.height) : constraints.minHeight;
    return {w, h};
}

// ============================================================================
// WrapContentPolicy
// ============================================================================

MeasureResult WrapContentPolicy::Measure(LayoutNodeVec children, Constraints constraints) {
    Constraints childConstraints = constraints;
    childConstraints.minWidth = 0;
    childConstraints.minHeight = 0;
    if (config_.unbounded) {
        childConstraints.maxWidth = Infinity;
        childConstraints.maxHeight = Infinity;
    }
    if (!children.empty()) {
        auto placeable = children[0]->measure(childConstraints);
        // If child is smaller than parent's min, align within the min-sized space.
        int parentW = constraints.constrainWidth(placeable.width());
        int parentH = constraints.constrainHeight(placeable.height());
        int x = align(config_.horizontalAlignment, placeable.width(), parentW);
        int y = align(config_.verticalAlignment, placeable.height(), parentH);
        placeable.placeAt(x, y);
        return {parentW, parentH};
    }
    return {constraints.minWidth, constraints.minHeight};
}

// ============================================================================
// DefaultMinSizePolicy
// ============================================================================

MeasureResult DefaultMinSizePolicy::Measure(LayoutNodeVec children, Constraints constraints) {
    Constraints childConstraints = constraints;
    if (childConstraints.minWidth == 0 && config_.minWidth > 0) {
        childConstraints.minWidth = std::min(config_.minWidth, childConstraints.maxWidth);
    }
    if (childConstraints.minHeight == 0 && config_.minHeight > 0) {
        childConstraints.minHeight = std::min(config_.minHeight, childConstraints.maxHeight);
    }
    if (!children.empty()) {
        auto placeable = children[0]->measure(childConstraints);
        placeable.placeAt(0, 0);
        return {placeable.width(), placeable.height()};
    }
    return {childConstraints.minWidth, childConstraints.minHeight};
}

// ============================================================================
// SizeInPolicy
// ============================================================================

MeasureResult SizeInPolicy::Measure(LayoutNodeVec children, Constraints constraints) {
    Constraints childConstraints;
    childConstraints.minWidth = std::clamp(config_.minWidth, constraints.minWidth, constraints.maxWidth);
    childConstraints.maxWidth = std::clamp(config_.maxWidth, constraints.minWidth, constraints.maxWidth);
    childConstraints.minHeight = std::clamp(config_.minHeight, constraints.minHeight, constraints.maxHeight);
    childConstraints.maxHeight = std::clamp(config_.maxHeight, constraints.minHeight, constraints.maxHeight);
    // Ensure min <= max after clamping.
    if (childConstraints.minWidth > childConstraints.maxWidth) {
        childConstraints.minWidth = childConstraints.maxWidth;
    }
    if (childConstraints.minHeight > childConstraints.maxHeight) {
        childConstraints.minHeight = childConstraints.maxHeight;
    }
    if (!children.empty()) {
        auto placeable = children[0]->measure(childConstraints);
        placeable.placeAt(0, 0);
        return {placeable.width(), placeable.height()};
    }
    return {childConstraints.minWidth, childConstraints.minHeight};
}

// ============================================================================
// AspectRatioPolicy
// ============================================================================

MeasureResult AspectRatioPolicy::Measure(LayoutNodeVec children, Constraints constraints) {
    // Try to find a width/height pair that satisfies the aspect ratio and fits
    // within constraints. The priority order depends on matchHeightConstraintsFirst.
    auto satisfies = [&](int w, int h) -> bool {
        return w >= constraints.minWidth && w <= constraints.maxWidth &&
               h >= constraints.minHeight && h <= constraints.maxHeight;
    };

    struct Attempt { int width; int height; };
    Attempt attempts[4];

    if (!config_.matchHeightConstraintsFirst) {
        // Width-first priority
        attempts[0] = {constraints.maxWidth,
                       static_cast<int>(constraints.maxWidth / config_.ratio)};
        attempts[1] = {static_cast<int>(constraints.maxHeight * config_.ratio),
                       constraints.maxHeight};
        attempts[2] = {constraints.minWidth,
                       static_cast<int>(constraints.minWidth / config_.ratio)};
        attempts[3] = {static_cast<int>(constraints.minHeight * config_.ratio),
                       constraints.minHeight};
    } else {
        // Height-first priority
        attempts[0] = {static_cast<int>(constraints.maxHeight * config_.ratio),
                       constraints.maxHeight};
        attempts[1] = {constraints.maxWidth,
                       static_cast<int>(constraints.maxWidth / config_.ratio)};
        attempts[2] = {static_cast<int>(constraints.minHeight * config_.ratio),
                       constraints.minHeight};
        attempts[3] = {constraints.minWidth,
                       static_cast<int>(constraints.minWidth / config_.ratio)};
    }

    int resolvedW = constraints.minWidth;
    int resolvedH = constraints.minHeight;

    for (auto& a : attempts) {
        // Skip unbounded attempts
        if (a.width == Infinity || a.height == Infinity) continue;
        if (satisfies(a.width, a.height)) {
            resolvedW = a.width;
            resolvedH = a.height;
            break;
        }
    }

    Constraints childConstraints = {resolvedW, resolvedW, resolvedH, resolvedH};
    if (!children.empty()) {
        auto placeable = children[0]->measure(childConstraints);
        placeable.placeAt(0, 0);
        return {placeable.width(), placeable.height()};
    }
    return {resolvedW, resolvedH};
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

LayoutNode* FlowRow(FlowRowConfig config, LayoutNodeVec children) {
    return new LayoutNode(new FlowRowPolicy(config), children);
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

LayoutNode* FlowRow(LayoutNodeVec children) {
    return FlowRow({}, children);
}

LayoutNode* FlowColumn(FlowColumnConfig config, LayoutNodeVec children) {
    return new LayoutNode(new FlowColumnPolicy(config), children);
}

LayoutNode* FlowColumn(LayoutNodeVec children) {
    return FlowColumn({}, children);
}

LayoutNode* Layout(MeasurePolicy* policy, LayoutNodeVec children) {
    return new LayoutNode(policy, children);
}

LayoutNode* FillMaxSize(LayoutNode* child, FillMaxSizeConfig config) {
    return new LayoutNode(new FillMaxSizePolicy(config), {child});
}

LayoutNode* FillMaxWidth(LayoutNode* child, float fraction) {
    return FillMaxSize(child, {.widthFraction = fraction, .heightFraction = -1.0f});
}

LayoutNode* FillMaxHeight(LayoutNode* child, float fraction) {
    return FillMaxSize(child, {.widthFraction = -1.0f, .heightFraction = fraction});
}

LayoutNode* Size(LayoutNode* child, int width, int height) {
    return new LayoutNode(new SizePolicy({.width = width, .height = height}), {child});
}

LayoutNode* Width(LayoutNode* child, int width) {
    return new LayoutNode(new SizePolicy({.width = width}), {child});
}

LayoutNode* Height(LayoutNode* child, int height) {
    return new LayoutNode(new SizePolicy({.height = height}), {child});
}

LayoutNode* RequiredSize(LayoutNode* child, int width, int height) {
    return new LayoutNode(new RequiredSizePolicy({.width = width, .height = height}), {child});
}

LayoutNode* WrapContent(LayoutNode* child, WrapContentConfig config) {
    return new LayoutNode(new WrapContentPolicy(config), {child});
}

LayoutNode* DefaultMinSize(LayoutNode* child, DefaultMinSizeConfig config) {
    return new LayoutNode(new DefaultMinSizePolicy(config), {child});
}

LayoutNode* SizeIn(LayoutNode* child, SizeInConfig config) {
    return new LayoutNode(new SizeInPolicy(config), {child});
}

LayoutNode* AspectRatio(LayoutNode* child, AspectRatioConfig config) {
    return new LayoutNode(new AspectRatioPolicy(config), {child});
}

LayoutNode* AspectRatio(LayoutNode* child, float ratio) {
    return AspectRatio(child, {.ratio = ratio});
}

} // namespace compose
