#pragma once

/// C++ port of GPUI's platform abstraction layer.
/// Defines interfaces for windowing, input, and rendering that are
/// implemented per-platform (macOS/Windows/Linux).

#include <gpui/geometry.h>
#include <gpui/input.h>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace gpui {

// Forward declarations
class TextSystem;

// ─── Display ───────────────────────────────────────────────────────────────

/// Represents a physical display/monitor.
struct DisplayInfo {
    uint32_t id = 0;
    Bounds<Pixels> bounds;         // Screen area in logical pixels
    Bounds<Pixels> work_area;      // Usable area (excluding taskbar/dock)
    float scale_factor = 1.0f;     // HiDPI scale
    std::string name;
};

// ─── Window Configuration ──────────────────────────────────────────────────

/// The kind of window to create.
enum class WindowKind {
    Normal,
    PopUp,
    Panel,      // macOS panel/utility window
};

/// Window decoration style.
enum class WindowDecorations {
    Server,     // Platform-managed decorations (default)
    Client,     // Custom title bar (client-side decorations)
};

/// Titlebar configuration.
struct TitlebarOptions {
    std::optional<std::string> title;
    bool appears_transparent = false;   // macOS: content extends into titlebar
    std::optional<float> traffic_light_position;  // macOS: custom button position
};

/// Window appearance (light/dark mode).
enum class WindowAppearance {
    Light,
    Dark,
};

/// The background style of the window.
enum class WindowBackground {
    Opaque,
    Transparent,
    Blurred,    // macOS vibrancy / Windows Mica
};

/// Configuration for creating a new window.
struct WindowOptions {
    WindowKind kind = WindowKind::Normal;
    std::optional<Bounds<Pixels>> bounds;
    std::optional<TitlebarOptions> titlebar;
    WindowDecorations decorations = WindowDecorations::Server;
    WindowBackground background = WindowBackground::Opaque;
    std::optional<uint32_t> display_id;
    bool resizable = true;
    bool show = true;
    bool focus = true;
    std::optional<Size<Pixels>> min_size;
};

// ─── Platform Window ───────────────────────────────────────────────────────

/// Abstract interface for a platform window.
class PlatformWindow {
public:
    virtual ~PlatformWindow() = default;

    /// Get the content bounds in logical pixels.
    virtual Bounds<Pixels> bounds() const = 0;

    /// Set the window position and size.
    virtual void set_bounds(Bounds<Pixels> bounds) = 0;

    /// Get the display scale factor.
    virtual float scale_factor() const = 0;

    /// Get the window title.
    virtual std::string title() const = 0;

    /// Set the window title.
    virtual void set_title(const std::string& title) = 0;

    /// Show or hide the window.
    virtual void set_visible(bool visible) = 0;

    /// Minimize the window.
    virtual void minimize() = 0;

    /// Maximize/zoom the window.
    virtual void maximize() = 0;

    /// Bring the window to the front.
    virtual void activate() = 0;

    /// Check if the window is focused.
    virtual bool is_active() const = 0;

    /// Check if the window is fullscreen.
    virtual bool is_fullscreen() const = 0;

    /// Toggle fullscreen mode.
    virtual void toggle_fullscreen() = 0;

    /// Set the cursor style.
    virtual void set_cursor(CursorStyle cursor) = 0;

    /// Invalidate the window, requesting a repaint.
    virtual void invalidate() = 0;

    /// Get the current appearance (light/dark).
    virtual WindowAppearance appearance() const = 0;

    /// Get the display this window is on.
    virtual DisplayInfo display() const = 0;

    // ─── Event callbacks ───────────────────────────────────────────────

    using InputCallback = std::function<bool(InputEvent)>;
    using ResizeCallback = std::function<void(Size<Pixels>)>;
    using CloseCallback = std::function<bool()>;
    using MoveCallback = std::function<void()>;
    using ActiveCallback = std::function<void(bool)>;
    using AppearanceCallback = std::function<void(WindowAppearance)>;

    virtual void on_input(InputCallback callback) = 0;
    virtual void on_resize(ResizeCallback callback) = 0;
    virtual void on_close(CloseCallback callback) = 0;
    virtual void on_move(MoveCallback callback) = 0;
    virtual void on_active_change(ActiveCallback callback) = 0;
    virtual void on_appearance_change(AppearanceCallback callback) = 0;

    /// Request that the window is repainted. The callback provides the
    /// scene to paint into.
    using PaintCallback = std::function<void()>;
    virtual void on_paint(PaintCallback callback) = 0;
};

// ─── Platform ──────────────────────────────────────────────────────────────

/// Abstract interface for the host platform.
/// Manages the application lifecycle, windows, and system integration.
class Platform {
public:
    virtual ~Platform() = default;

    /// Run the platform event loop (blocks until quit).
    virtual void run(std::function<void()> on_ready) = 0;

    /// Request the application to quit.
    virtual void quit() = 0;

    /// Create a new platform window.
    virtual std::unique_ptr<PlatformWindow> open_window(const WindowOptions& options) = 0;

    /// Get all connected displays.
    virtual std::vector<DisplayInfo> displays() const = 0;

    /// Get the primary display.
    virtual DisplayInfo primary_display() const = 0;

    /// Get the current mouse position in screen coordinates.
    virtual Point<Pixels> mouse_position() const = 0;

    /// Set the global cursor style.
    virtual void set_cursor(CursorStyle cursor) = 0;

    /// Open a URL in the default browser.
    virtual void open_url(const std::string& url) = 0;

    /// Read from the system clipboard.
    virtual std::optional<std::string> clipboard_text() const = 0;

    /// Write to the system clipboard.
    virtual void set_clipboard_text(const std::string& text) = 0;

    /// Get the system text rendering engine.
    virtual TextSystem& text_system() = 0;

    /// Get the system appearance (light/dark).
    virtual WindowAppearance appearance() const = 0;

    /// Register a callback for when appearance changes system-wide.
    virtual void on_appearance_change(std::function<void(WindowAppearance)> callback) = 0;

    /// Dispatch a callback on the main thread.
    virtual void dispatch_on_main(std::function<void()> callback) = 0;
};

/// Create the platform implementation for the current OS.
std::unique_ptr<Platform> create_platform();

} // namespace gpui
