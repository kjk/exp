#include <gpui/scene.h>
#include <algorithm>

namespace gpui {

std::vector<PrimitiveBatch> Scene::batches() const {
    std::vector<PrimitiveBatch> result;
    if (primitives_.empty()) return result;

    // Group consecutive primitives of the same type into batches.
    PrimitiveBatch current;
    current.kind = static_cast<PrimitiveBatch::Kind>(primitives_[0].content.index());

    for (const auto& prim : primitives_) {
        auto kind = static_cast<PrimitiveBatch::Kind>(prim.content.index());
        if (kind != current.kind && !current.primitives.empty()) {
            result.push_back(std::move(current));
            current = PrimitiveBatch{};
            current.kind = kind;
        }
        current.primitives.push_back(&prim);
    }

    if (!current.primitives.empty()) {
        result.push_back(std::move(current));
    }

    return result;
}

} // namespace gpui
