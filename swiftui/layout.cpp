#include "layout.h"

#include <algorithm>
#include <numeric>
#include <vector>

namespace swiftui {

// ============================================================================
// Alignment helpers
// ============================================================================

float alignH(HorizontalAlignment align, float childWidth, float parentWidth) {
    switch (align) {
    case HorizontalAlignment::leading: return 0;
    case HorizontalAlignment::center: return (parentWidth - childWidth) / 2;
    case HorizontalAlignment::trailing: return parentWidth - childWidth;
    }
    return 0;
}

float alignV(VerticalAlignment align, float childHeight, float parentHeight) {
    switch (align) {
    case VerticalAlignment::top: return 0;
    case VerticalAlignment::center: return (parentHeight - childHeight) / 2;
    case VerticalAlignment::bottom: return parentHeight - childHeight;
    case VerticalAlignment::firstTextBaseline: return 0;  // fallback: same as top
    case VerticalAlignment::lastTextBaseline: return 0;
    }
    return 0;
}

// ============================================================================
// ViewDimensions
// ============================================================================

float ViewDimensions::operator[](HorizontalAlignment guide) const {
    switch (guide) {
    case HorizontalAlignment::leading:
        return explicitLeading.value_or(0);
    case HorizontalAlignment::center:
        return explicitHCenter.value_or(width / 2);
    case HorizontalAlignment::trailing:
        return explicitTrailing.value_or(width);
    }
    return 0;
}

float ViewDimensions::operator[](VerticalAlignment guide) const {
    switch (guide) {
    case VerticalAlignment::top:
        return explicitTop.value_or(0);
    case VerticalAlignment::center:
        return explicitVCenter.value_or(height / 2);
    case VerticalAlignment::bottom:
        return explicitBottom.value_or(height);
    case VerticalAlignment::firstTextBaseline:
        return explicitBottom.value_or(height);  // non-text default
    case VerticalAlignment::lastTextBaseline:
        return explicitBottom.value_or(height);
    }
    return 0;
}

// ============================================================================
// ViewSpacing
// ============================================================================

float ViewSpacing::distance(const ViewSpacing& next, Axis axis) const {
    if (axis == Axis::horizontal) {
        return std::max(trailing, next.leading);
    } else {
        return std::max(bottom, next.top);
    }
}

void ViewSpacing::formUnion(const ViewSpacing& other) {
    top = std::max(top, other.top);
    leading = std::max(leading, other.leading);
    bottom = std::max(bottom, other.bottom);
    trailing = std::max(trailing, other.trailing);
}

// ============================================================================
// Layout (base class defaults)
// ============================================================================

void* Layout::makeCache(LayoutSubviews /*subviews*/) {
    return nullptr;
}

void Layout::destroyCache(void* /*cache*/) {
    // Default: no cache to destroy.
}

ViewSpacing Layout::spacing(LayoutSubviews subviews, void* /*cache*/) {
    ViewSpacing result = ViewSpacing::zero();
    for (auto& sv : subviews) {
        result.formUnion(sv.spacing());
    }
    return result;
}

// ============================================================================
// LayoutSubview
// ============================================================================

Size LayoutSubview::sizeThatFits(ProposedViewSize proposal) const {
    return node_->measure(proposal);
}

ViewDimensions LayoutSubview::dimensions(ProposedViewSize proposal) const {
    node_->measure(proposal);
    return node_->dimensions();
}

void LayoutSubview::place(Point position, UnitPoint anchor, ProposedViewSize proposal) const {
    // Resolve the anchor offset: the anchor point on the child maps to the position.
    float adjustedX = position.x - anchor.x * node_->width();
    float adjustedY = position.y - anchor.y * node_->height();
    node_->placeAt(adjustedX, adjustedY);

    // Trigger recursive placement of this child's subtree.
    node_->doPlace(proposal);
}

float LayoutSubview::priority() const {
    return node_->priority();
}

ViewSpacing LayoutSubview::spacing() const {
    return node_->spacing();
}

// ============================================================================
// LayoutNode
// ============================================================================

LayoutNode::LayoutNode(Layout* layout, std::initializer_list<LayoutNode*> children)
    : layout_(layout),
      childCount_(static_cast<int>(children.size())),
      children_(children.size() ? new LayoutNode*[children.size()] : nullptr) {
    int i = 0;
    for (auto* child : children) {
        children_[i++] = child;
    }
}

LayoutNode::~LayoutNode() {
    if (layout_ && cache_) {
        layout_->destroyCache(cache_);
    }
    delete layout_;
    delete[] children_;
}

void LayoutNode::layout(ProposedViewSize proposal) {
    measure(proposal);
    placeAt(0, 0);
    doPlace(proposal);
}

Size LayoutNode::measure(ProposedViewSize proposal) {
    if (!layout_) {
        width_ = 0;
        height_ = 0;
        dims_ = {};
        return {0, 0};
    }

    // Build subview proxies for children.
    auto* proxies = childCount_ > 0 ? new LayoutSubview[childCount_] : nullptr;
    for (int i = 0; i < childCount_; i++) {
        proxies[i] = LayoutSubview(children_[i]);
    }
    LayoutSubviews subviews(childCount_, proxies);

    // Initialize cache on first measurement.
    if (!cache_) {
        cache_ = layout_->makeCache(subviews);
    }

    Size result = layout_->sizeThatFits(proposal, subviews, cache_);
    width_ = result.width;
    height_ = result.height;
    dims_.width = width_;
    dims_.height = height_;

    delete[] proxies;
    return result;
}

void LayoutNode::doPlace(ProposedViewSize proposal) {
    if (!layout_ || childCount_ == 0) return;

    auto* proxies = new LayoutSubview[childCount_];
    for (int i = 0; i < childCount_; i++) {
        proxies[i] = LayoutSubview(children_[i]);
    }
    LayoutSubviews subviews(childCount_, proxies);

    Rect bounds = {0, 0, width_, height_};
    layout_->placeSubviews(bounds, proposal, subviews, cache_);

    delete[] proxies;
}

void LayoutNode::placeAt(float x, float y) {
    x_ = x;
    y_ = y;
}

LayoutNode* LayoutNode::setAlignmentGuide(HorizontalAlignment guide, float value) {
    switch (guide) {
    case HorizontalAlignment::leading: dims_.explicitLeading = value; break;
    case HorizontalAlignment::center: dims_.explicitHCenter = value; break;
    case HorizontalAlignment::trailing: dims_.explicitTrailing = value; break;
    }
    return this;
}

LayoutNode* LayoutNode::setAlignmentGuide(VerticalAlignment guide, float value) {
    switch (guide) {
    case VerticalAlignment::top: dims_.explicitTop = value; break;
    case VerticalAlignment::center: dims_.explicitVCenter = value; break;
    case VerticalAlignment::bottom: dims_.explicitBottom = value; break;
    case VerticalAlignment::firstTextBaseline: break;  // not stored separately
    case VerticalAlignment::lastTextBaseline: break;
    }
    return this;
}

void LayoutNode::collectBounds(std::vector<BoundsRect>& out, float offsetX, float offsetY) const {
    float absX = offsetX + x_;
    float absY = offsetY + y_;
    if (childCount_ == 0) {
        out.push_back({absX, absY, width_, height_});
    }
    for (int i = 0; i < childCount_; i++) {
        children_[i]->collectBounds(out, absX, absY);
    }
}

void freeTree(LayoutNode* node) {
    if (!node) return;
    for (int i = 0; i < node->childCount(); i++) {
        freeTree(node->children()[i]);
    }
    delete node;
}

// ============================================================================
// LeafLayout
// ============================================================================

Size LeafLayout::sizeThatFits(ProposedViewSize /*proposal*/, LayoutSubviews /*subviews*/,
                              void* /*cache*/) {
    // A fixed-size leaf ignores the proposal and returns its configured size.
    return {config_.width, config_.height};
}

void LeafLayout::placeSubviews(Rect /*bounds*/, ProposedViewSize /*proposal*/,
                               LayoutSubviews /*subviews*/, void* /*cache*/) {
    // Leaf has no children to place.
}

// ============================================================================
// FlexibleLeafLayout
// ============================================================================

static float resolveFlexible(std::optional<float> proposed, float minVal,
                             float idealVal, float maxVal) {
    if (!proposed.has_value()) {
        return idealVal;  // unspecified -> ideal
    }
    float p = proposed.value();
    if (p <= 0) return minVal;
    if (std::isinf(p)) return maxVal;
    return std::clamp(p, minVal, maxVal);
}

Size FlexibleLeafLayout::sizeThatFits(ProposedViewSize proposal,
                                       LayoutSubviews /*subviews*/, void* /*cache*/) {
    float w = resolveFlexible(proposal.width, config_.minSize.width,
                              config_.idealSize.width, config_.maxSize.width);
    float h = resolveFlexible(proposal.height, config_.minSize.height,
                              config_.idealSize.height, config_.maxSize.height);
    return {w, h};
}

void FlexibleLeafLayout::placeSubviews(Rect /*bounds*/, ProposedViewSize /*proposal*/,
                                        LayoutSubviews /*subviews*/, void* /*cache*/) {
    // No children.
}

// ============================================================================
// HStackLayout
// ============================================================================

// Compute the spacing between each adjacent pair of subviews.
static std::vector<float> computeSpacing(LayoutSubviews subviews,
                                          std::optional<float> explicitSpacing, Axis axis) {
    int n = subviews.size();
    if (n <= 1) return {};

    std::vector<float> spacings(n - 1);
    for (int i = 0; i < n - 1; i++) {
        if (explicitSpacing.has_value()) {
            spacings[i] = explicitSpacing.value();
        } else {
            spacings[i] = subviews[i].spacing().distance(subviews[i + 1].spacing(), axis);
        }
    }
    return spacings;
}

static float sumSpacing(const std::vector<float>& spacings) {
    float total = 0;
    for (float s : spacings) total += s;
    return total;
}

Size HStackLayout::sizeThatFits(ProposedViewSize proposal, LayoutSubviews subviews,
                                 void* /*cache*/) {
    int n = subviews.size();
    if (n == 0) return {0, 0};

    auto spacings = computeSpacing(subviews, config_.spacing, Axis::horizontal);
    float totalSpacing = sumSpacing(spacings);

    // Determine the width budget for children.
    float availableWidth = proposal.width.has_value()
                               ? proposal.width.value() - totalSpacing
                               : Inf;

    // Measure children: offer each a share of remaining width.
    std::vector<Size> sizes(n);
    float usedWidth = 0;
    int remaining = n;

    // Sort by priority (higher priority measured first to get space preference).
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return subviews[a].priority() > subviews[b].priority();
    });

    for (int idx : order) {
        float share = (availableWidth - usedWidth) / remaining;
        if (!std::isfinite(share)) share = Inf;
        float offerWidth = std::max(0.0f, share);

        ProposedViewSize childProposal(
            std::isfinite(offerWidth) ? std::optional<float>(offerWidth) : std::nullopt,
            proposal.height);
        sizes[idx] = subviews[idx].sizeThatFits(childProposal);
        usedWidth += sizes[idx].width;
        remaining--;
    }

    float totalWidth = usedWidth + totalSpacing;
    float maxHeight = 0;
    for (int i = 0; i < n; i++) {
        maxHeight = std::max(maxHeight, sizes[i].height);
    }

    return {totalWidth, maxHeight};
}

void HStackLayout::placeSubviews(Rect bounds, ProposedViewSize proposal,
                                   LayoutSubviews subviews, void* /*cache*/) {
    int n = subviews.size();
    if (n == 0) return;

    auto spacings = computeSpacing(subviews, config_.spacing, Axis::horizontal);
    float totalSpacing = sumSpacing(spacings);
    float availableWidth = bounds.width - totalSpacing;

    std::vector<Size> sizes(n);
    float usedWidth = 0;
    int remaining = n;

    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return subviews[a].priority() > subviews[b].priority();
    });

    for (int idx : order) {
        float share = (availableWidth - usedWidth) / remaining;
        float offerWidth = std::max(0.0f, share);
        ProposedViewSize childProposal(
            std::isfinite(offerWidth) ? std::optional<float>(offerWidth) : std::nullopt,
            proposal.height);
        sizes[idx] = subviews[idx].sizeThatFits(childProposal);
        usedWidth += sizes[idx].width;
        remaining--;
    }

    // Place children left-to-right with cross-axis alignment.
    float x = bounds.minX();
    for (int i = 0; i < n; i++) {
        float crossOffset = alignV(config_.alignment, sizes[i].height, bounds.height);
        float y = bounds.minY() + crossOffset;

        ProposedViewSize childProposal(sizes[i].width, sizes[i].height);
        subviews[i].place({x, y}, UnitPoint::topLeading(), childProposal);

        x += sizes[i].width;
        if (i < n - 1) x += spacings[i];
    }
}

// ============================================================================
// VStackLayout
// ============================================================================

Size VStackLayout::sizeThatFits(ProposedViewSize proposal, LayoutSubviews subviews,
                                 void* /*cache*/) {
    int n = subviews.size();
    if (n == 0) return {0, 0};

    auto spacings = computeSpacing(subviews, config_.spacing, Axis::vertical);
    float totalSpacing = sumSpacing(spacings);

    float availableHeight = proposal.height.has_value()
                                ? proposal.height.value() - totalSpacing
                                : Inf;

    std::vector<Size> sizes(n);
    float usedHeight = 0;
    int remaining = n;

    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return subviews[a].priority() > subviews[b].priority();
    });

    for (int idx : order) {
        float share = (availableHeight - usedHeight) / remaining;
        if (!std::isfinite(share)) share = Inf;
        float offerHeight = std::max(0.0f, share);

        ProposedViewSize childProposal(
            proposal.width,
            std::isfinite(offerHeight) ? std::optional<float>(offerHeight) : std::nullopt);
        sizes[idx] = subviews[idx].sizeThatFits(childProposal);
        usedHeight += sizes[idx].height;
        remaining--;
    }

    float totalHeight = usedHeight + totalSpacing;
    float maxWidth = 0;
    for (int i = 0; i < n; i++) {
        maxWidth = std::max(maxWidth, sizes[i].width);
    }

    return {maxWidth, totalHeight};
}

void VStackLayout::placeSubviews(Rect bounds, ProposedViewSize proposal,
                                   LayoutSubviews subviews, void* /*cache*/) {
    int n = subviews.size();
    if (n == 0) return;

    auto spacings = computeSpacing(subviews, config_.spacing, Axis::vertical);
    float totalSpacing = sumSpacing(spacings);
    float availableHeight = bounds.height - totalSpacing;

    std::vector<Size> sizes(n);
    float usedHeight = 0;
    int remaining = n;

    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return subviews[a].priority() > subviews[b].priority();
    });

    for (int idx : order) {
        float share = (availableHeight - usedHeight) / remaining;
        float offerHeight = std::max(0.0f, share);
        ProposedViewSize childProposal(
            proposal.width,
            std::isfinite(offerHeight) ? std::optional<float>(offerHeight) : std::nullopt);
        sizes[idx] = subviews[idx].sizeThatFits(childProposal);
        usedHeight += sizes[idx].height;
        remaining--;
    }

    // Place children top-to-bottom with cross-axis alignment.
    float y = bounds.minY();
    for (int i = 0; i < n; i++) {
        float crossOffset = alignH(config_.alignment, sizes[i].width, bounds.width);
        float x = bounds.minX() + crossOffset;

        ProposedViewSize childProposal(sizes[i].width, sizes[i].height);
        subviews[i].place({x, y}, UnitPoint::topLeading(), childProposal);

        y += sizes[i].height;
        if (i < n - 1) y += spacings[i];
    }
}

// ============================================================================
// ZStackLayout
// ============================================================================

Size ZStackLayout::sizeThatFits(ProposedViewSize proposal, LayoutSubviews subviews,
                                 void* /*cache*/) {
    int n = subviews.size();
    if (n == 0) return {0, 0};

    float maxWidth = 0;
    float maxHeight = 0;
    for (auto& sv : subviews) {
        Size sz = sv.sizeThatFits(proposal);
        maxWidth = std::max(maxWidth, sz.width);
        maxHeight = std::max(maxHeight, sz.height);
    }
    return {maxWidth, maxHeight};
}

void ZStackLayout::placeSubviews(Rect bounds, ProposedViewSize proposal,
                                   LayoutSubviews subviews, void* /*cache*/) {
    for (auto& sv : subviews) {
        Size sz = sv.sizeThatFits(proposal);
        float x = bounds.minX() + alignH(config_.alignment.horizontal, sz.width, bounds.width);
        float y = bounds.minY() + alignV(config_.alignment.vertical, sz.height, bounds.height);
        ProposedViewSize childProposal(sz.width, sz.height);
        sv.place({x, y}, UnitPoint::topLeading(), childProposal);
    }
}

// ============================================================================
// Builder functions
// ============================================================================

LayoutNode* Leaf(LeafConfig config) {
    return new LayoutNode(new LeafLayout(config));
}

LayoutNode* Leaf(float width, float height) {
    return Leaf({width, height});
}

LayoutNode* FlexibleLeaf(FlexibleLeafConfig config) {
    auto* node = new LayoutNode(new FlexibleLeafLayout(config));
    node->setIdealSize(config.idealSize);
    return node;
}

LayoutNode* HStack(HStackConfig config, std::initializer_list<LayoutNode*> children) {
    return new LayoutNode(new HStackLayout(config), children);
}

LayoutNode* VStack(VStackConfig config, std::initializer_list<LayoutNode*> children) {
    return new LayoutNode(new VStackLayout(config), children);
}

LayoutNode* ZStack(ZStackConfig config, std::initializer_list<LayoutNode*> children) {
    return new LayoutNode(new ZStackLayout(config), children);
}

LayoutNode* HStack(std::initializer_list<LayoutNode*> children) {
    return HStack({}, children);
}

LayoutNode* VStack(std::initializer_list<LayoutNode*> children) {
    return VStack({}, children);
}

LayoutNode* ZStack(std::initializer_list<LayoutNode*> children) {
    return ZStack({}, children);
}

} // namespace swiftui
