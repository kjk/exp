#pragma once

/// C++ port of GPUI's input module.
/// Defines keyboard and mouse event types and input handling infrastructure.

#include <gpui/geometry.h>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace gpui {

// ─── Modifier Keys ─────────────────────────────────────────────────────────

/// Modifier key state.
struct Modifiers {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    bool meta = false;   // Cmd on macOS, Win on Windows
    bool fn_key = false;

    constexpr bool is_empty() const {
        return !ctrl && !alt && !shift && !meta && !fn_key;
    }

    constexpr bool operator==(const Modifiers& other) const {
        return ctrl == other.ctrl && alt == other.alt &&
               shift == other.shift && meta == other.meta && fn_key == other.fn_key;
    }

    /// Check if the platform command modifier is held (Cmd on macOS, Ctrl elsewhere).
    constexpr bool command() const {
#ifdef __APPLE__
        return meta;
#else
        return ctrl;
#endif
    }
};

// ─── Mouse Button ──────────────────────────────────────────────────────────

/// Which mouse button was pressed.
enum class MouseButton {
    Left,
    Right,
    Middle,
    Back,
    Forward,
};

// ─── Keystroke ─────────────────────────────────────────────────────────────

/// A keystroke representing a single key press with modifiers.
struct Keystroke {
    std::string key;         // The key name (e.g., "a", "enter", "space")
    Modifiers modifiers;
    std::string ime_key;     // Input method editor composed key, if any

    bool operator==(const Keystroke& other) const {
        return key == other.key && modifiers == other.modifiers;
    }
};

// ─── Keyboard Events ───────────────────────────────────────────────────────

/// A key was pressed.
struct KeyDownEvent {
    Keystroke keystroke;
    bool is_held = false;    // True if this is a repeat from holding the key
};

/// A key was released.
struct KeyUpEvent {
    Keystroke keystroke;
};

/// Modifier keys changed state.
struct ModifiersChangedEvent {
    Modifiers modifiers;
};

// ─── Mouse Events ──────────────────────────────────────────────────────────

/// A mouse button was pressed.
struct MouseDownEvent {
    MouseButton button = MouseButton::Left;
    Point<Pixels> position;
    Modifiers modifiers;
    uint32_t click_count = 1;
    bool first_mouse = false; // macOS: first click to bring window to front
};

/// A mouse button was released.
struct MouseUpEvent {
    MouseButton button = MouseButton::Left;
    Point<Pixels> position;
    Modifiers modifiers;
    uint32_t click_count = 1;
};

/// The mouse cursor moved.
struct MouseMoveEvent {
    Point<Pixels> position;
    std::optional<MouseButton> pressed_button;
    Modifiers modifiers;
};

/// The scroll wheel was used.
struct ScrollWheelEvent {
    Point<Pixels> position;
    Point<Pixels> delta;
    Modifiers modifiers;
    bool precise = false;   // True for trackpad/smooth scrolling

    enum class Phase { Began, Changed, Ended };
    std::optional<Phase> phase;
};

/// Force/pressure on a trackpad (macOS).
struct MousePressureEvent {
    Point<Pixels> position;
    float pressure = 0.0f;
    int stage = 0;
};

/// The mouse cursor exited the window.
struct MouseExitEvent {
    Point<Pixels> position;
    std::optional<MouseButton> pressed_button;
    Modifiers modifiers;
};

// ─── Click Event ───────────────────────────────────────────────────────────

/// A click (mouse down + mouse up) on an element.
struct ClickEvent {
    MouseDownEvent down;
    MouseUpEvent up;
};

// ─── File Drop Events ──────────────────────────────────────────────────────

/// File drag/drop event.
struct FileDropEvent {
    enum class Phase {
        Entered,   // Files entered the window
        Pending,   // Files hovering
        Submit,    // Files dropped
        Exited,    // Files left the window
    };

    Phase phase = Phase::Entered;
    Point<Pixels> position;
    std::vector<std::string> file_paths;
};

// ─── Unified Input Event ───────────────────────────────────────────────────

/// A type-erased input event (any of the above event types).
using InputEvent = std::variant<
    KeyDownEvent,
    KeyUpEvent,
    ModifiersChangedEvent,
    MouseDownEvent,
    MouseUpEvent,
    MouseMoveEvent,
    ScrollWheelEvent,
    MousePressureEvent,
    MouseExitEvent,
    FileDropEvent
>;

/// Get the position from a mouse-related event, if applicable.
inline std::optional<Point<Pixels>> event_position(const InputEvent& event) {
    return std::visit([](auto&& e) -> std::optional<Point<Pixels>> {
        using T = std::decay_t<decltype(e)>;
        if constexpr (requires { e.position; }) {
            return e.position;
        } else {
            return std::nullopt;
        }
    }, event);
}

// ─── Cursor Style ──────────────────────────────────────────────────────────

/// Mouse cursor appearance.
enum class CursorStyle {
    Arrow,
    IBeam,
    Crosshair,
    ClosedHand,
    OpenHand,
    PointingHand,
    ResizeLeft,
    ResizeRight,
    ResizeLeftRight,
    ResizeUp,
    ResizeDown,
    ResizeUpDown,
    DisappearingItem,
    OperationNotAllowed,
    DragLink,
    DragCopy,
    ContextualMenu,
};

} // namespace gpui
