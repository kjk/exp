#pragma once

/// C++ port of GPUI's text element.
/// Renders styled text content using the platform text system.

#include <gpui/element.h>
#include <gpui/styled.h>
#include <gpui/text_system.h>
#include <string>
#include <vector>

namespace gpui {

// ─── Text Run Builder ──────────────────────────────────────────────────────

/// A styled span of text within a text element.
struct StyledText {
    std::string text;
    TextStyle style;

    StyledText(std::string t) : text(std::move(t)) {}
    StyledText(std::string t, TextStyle s) : text(std::move(t)), style(std::move(s)) {}
    StyledText(const char* t) : text(t) {}
};

// ─── Text Element ──────────────────────────────────────────────────────────

/// An element that renders text with optional styling.
class TextElement : public Element, public Styled<TextElement> {
public:
    TextElement() = default;
    explicit TextElement(std::string text) {
        runs_.push_back(StyledText(std::move(text)));
    }
    explicit TextElement(std::vector<StyledText> runs) : runs_(std::move(runs)) {}

    // ─── Content ───────────────────────────────────────────────────────

    /// Set the text content (replaces all runs with a single unstyled run).
    TextElement& text(std::string content) {
        runs_.clear();
        runs_.push_back(StyledText(std::move(content)));
        return *this;
    }

    /// Add a styled text run.
    TextElement& run(StyledText styled_text) {
        runs_.push_back(std::move(styled_text));
        return *this;
    }

    /// Set the maximum number of visible lines.
    TextElement& max_lines(std::size_t n) {
        max_lines_ = n;
        return *this;
    }

    // ─── Styled interface ──────────────────────────────────────────────

    StyleRefinement& style() { return style_; }
    const StyleRefinement& style() const { return style_; }

    // ─── Element interface ─────────────────────────────────────────────

    LayoutId request_layout(
        const std::optional<GlobalElementId>& global_id,
        Window& window, App& cx) override;

    void prepaint(
        const std::optional<GlobalElementId>& global_id,
        Bounds<Pixels> bounds, Window& window, App& cx) override;

    void paint(
        const std::optional<GlobalElementId>& global_id,
        Bounds<Pixels> bounds, Window& window, App& cx) override;

private:
    StyleRefinement style_;
    std::vector<StyledText> runs_;
    std::optional<std::size_t> max_lines_;

    // Cached layout
    std::optional<TextLayout> cached_layout_;
};

// ─── Convenience Functions ─────────────────────────────────────────────────

/// Create a text element from a string.
inline TextElement text(std::string content) {
    return TextElement(std::move(content));
}

/// Create a text element from a C string.
inline TextElement text(const char* content) {
    return TextElement(std::string(content));
}

/// Create a text element with multiple styled runs.
inline TextElement text(std::vector<StyledText> runs) {
    return TextElement(std::move(runs));
}

// ─── String to Element conversion ─────────────────────────────────────────

/// Allow strings to be used as elements directly (renders as text).
class StringElement : public Element {
public:
    explicit StringElement(std::string text) : text_(std::move(text)) {}

    LayoutId request_layout(
        const std::optional<GlobalElementId>& global_id,
        Window& window, App& cx) override;

    void prepaint(
        const std::optional<GlobalElementId>& global_id,
        Bounds<Pixels> bounds, Window& window, App& cx) override;

    void paint(
        const std::optional<GlobalElementId>& global_id,
        Bounds<Pixels> bounds, Window& window, App& cx) override;

private:
    std::string text_;
    TextElement inner_;
};

} // namespace gpui
