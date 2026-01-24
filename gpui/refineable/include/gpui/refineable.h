#pragma once

/// C++ port of Zed's `refineable` crate.
/// Provides a mechanism for cascading style overrides, similar to CSS specificity.
/// Fields wrapped in Refineable<T> can be selectively overridden while preserving
/// unset values from a base style.

#include <optional>
#include <type_traits>

namespace gpui {

/// A refineable value that can be optionally overridden.
/// When refined, the override takes precedence; otherwise the base value is used.
template <typename T>
using Refineable = std::optional<T>;

/// Concept for types that support refinement (merging partial overrides).
template <typename T>
concept IsRefineable = requires(T a, const T& b) {
    { a.refine(b) };
};

/// Refine a single optional field: if the refinement has a value, use it.
template <typename T>
void refine_field(std::optional<T>& base, const std::optional<T>& refinement) {
    if (refinement.has_value()) {
        base = refinement;
    }
}

/// Refine a non-optional field that itself is refineable (nested structs).
template <IsRefineable T>
void refine_value(T& base, const T& refinement) {
    base.refine(refinement);
}

/// Macro-like helper to define a refineable struct.
/// In C++ we use a `refine` method that applies overrides from another instance.
///
/// Example usage:
/// ```cpp
/// struct MyStyle {
///     std::optional<Color> background;
///     std::optional<float> opacity;
///
///     void refine(const MyStyle& other) {
///         refine_field(background, other.background);
///         refine_field(opacity, other.opacity);
///     }
/// };
/// ```

/// Helper to create a "refined" copy: base with overrides applied.
template <typename T>
    requires IsRefineable<T>
T refined(T base, const T& overrides) {
    base.refine(overrides);
    return base;
}

/// Convert a refineable struct to its resolved form by providing defaults.
/// Any unset optional fields will use the default value.
template <typename T>
T resolve_or(const std::optional<T>& field, const T& default_value) {
    return field.value_or(default_value);
}

} // namespace gpui
