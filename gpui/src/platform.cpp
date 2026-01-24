#include <gpui/platform.h>
#include <gpui/text_system.h>
#include <iostream>
#include <thread>

namespace gpui {

// ─── Stub Platform Implementation ──────────────────────────────────────────
// This provides a minimal platform implementation for testing and development.
// Real implementations would use Cocoa/AppKit, Win32, or GTK/Wayland.

class StubPlatformWindow : public PlatformWindow {
public:
    explicit StubPlatformWindow(const WindowOptions& options)
        : bounds_(options.bounds.value_or(Bounds<Pixels>{
              Point<Pixels>{Pixels{100}, Pixels{100}},
              Size<Pixels>{Pixels{800}, Pixels{600}}
          }))
        , title_(options.titlebar
                     ? options.titlebar->title.value_or("GPUI Window")
                     : "GPUI Window")
        , visible_(options.show) {}

    Bounds<Pixels> bounds() const override { return bounds_; }
    void set_bounds(Bounds<Pixels> bounds) override {
        bounds_ = bounds;
        if (resize_cb_) resize_cb_(bounds.size);
    }

    float scale_factor() const override { return 1.0f; }
    std::string title() const override { return title_; }
    void set_title(const std::string& title) override { title_ = title; }
    void set_visible(bool visible) override { visible_ = visible; }
    void minimize() override {}
    void maximize() override {}
    void activate() override { active_ = true; }
    bool is_active() const override { return active_; }
    bool is_fullscreen() const override { return false; }
    void toggle_fullscreen() override {}
    void set_cursor(CursorStyle) override {}
    void invalidate() override { if (paint_cb_) paint_cb_(); }
    WindowAppearance appearance() const override { return WindowAppearance::Dark; }
    DisplayInfo display() const override {
        return DisplayInfo{0, bounds_, bounds_, 1.0f, "Stub Display"};
    }

    void on_input(InputCallback callback) override { input_cb_ = std::move(callback); }
    void on_resize(ResizeCallback callback) override { resize_cb_ = std::move(callback); }
    void on_close(CloseCallback callback) override { close_cb_ = std::move(callback); }
    void on_move(MoveCallback callback) override { move_cb_ = std::move(callback); }
    void on_active_change(ActiveCallback callback) override { active_cb_ = std::move(callback); }
    void on_appearance_change(AppearanceCallback callback) override { appearance_cb_ = std::move(callback); }
    void on_paint(PaintCallback callback) override { paint_cb_ = std::move(callback); }

private:
    Bounds<Pixels> bounds_;
    std::string title_;
    bool visible_ = true;
    bool active_ = true;

    InputCallback input_cb_;
    ResizeCallback resize_cb_;
    CloseCallback close_cb_;
    MoveCallback move_cb_;
    ActiveCallback active_cb_;
    AppearanceCallback appearance_cb_;
    PaintCallback paint_cb_;
};

class StubPlatform : public Platform {
public:
    StubPlatform() : text_system_(create_text_system()) {}

    void run(std::function<void()> on_ready) override {
        on_ready();
        // In a real implementation, this would run the event loop.
        // For the stub, we just run the ready callback and return.
    }

    void quit() override {
        running_ = false;
    }

    std::unique_ptr<PlatformWindow> open_window(const WindowOptions& options) override {
        return std::make_unique<StubPlatformWindow>(options);
    }

    std::vector<DisplayInfo> displays() const override {
        return {primary_display()};
    }

    DisplayInfo primary_display() const override {
        return DisplayInfo{
            0,
            Bounds<Pixels>{Point<Pixels>{}, Size<Pixels>{Pixels{1920}, Pixels{1080}}},
            Bounds<Pixels>{Point<Pixels>{}, Size<Pixels>{Pixels{1920}, Pixels{1040}}},
            1.0f,
            "Primary Display"
        };
    }

    Point<Pixels> mouse_position() const override {
        return Point<Pixels>{Pixels{0}, Pixels{0}};
    }

    void set_cursor(CursorStyle) override {}

    void open_url(const std::string&) override {}

    std::optional<std::string> clipboard_text() const override {
        return clipboard_;
    }

    void set_clipboard_text(const std::string& text) override {
        clipboard_ = text;
    }

    TextSystem& text_system() override { return *text_system_; }

    WindowAppearance appearance() const override { return WindowAppearance::Dark; }

    void on_appearance_change(std::function<void(WindowAppearance)>) override {}

    void dispatch_on_main(std::function<void()> callback) override {
        callback(); // In stub, just run immediately
    }

private:
    bool running_ = true;
    std::string clipboard_;
    std::unique_ptr<TextSystem> text_system_;
};

std::unique_ptr<Platform> create_platform() {
    // TODO: Return platform-specific implementation based on OS
    return std::make_unique<StubPlatform>();
}

} // namespace gpui
