#include <gpui/app.h>
#include <gpui/window.h>

namespace gpui {

App::App() : platform_(create_platform()) {}

App::~App() = default;

WindowHandle App::open_window(const WindowOptions& options,
                               std::function<void(Window&, App&)> build_root) {
    WindowHandle handle{next_window_id_++};
    window_handles_.push_back(handle);

    auto platform_window = platform_->open_window(options);
    auto layout_engine = create_layout_engine();
    auto window = std::make_unique<Window>(
        std::move(platform_window), std::move(layout_engine));

    Window* window_ptr = window.get();
    windows_[handle.id] = std::move(window);

    if (build_root) {
        build_root(*window_ptr, *this);
    }

    return handle;
}

void App::update_window(WindowHandle handle,
                         std::function<void(Window&, App&)> callback) {
    auto it = windows_.find(handle.id);
    if (it != windows_.end()) {
        callback(*it->second, *this);
    }
}

void App::close_window(WindowHandle handle) {
    windows_.erase(handle.id);
    window_handles_.erase(
        std::remove(window_handles_.begin(), window_handles_.end(), handle),
        window_handles_.end());
}

void App::flush_effects() {
    while (!deferred_.empty()) {
        auto effects = std::move(deferred_);
        deferred_.clear();
        for (auto& effect : effects) {
            effect(*this);
        }
    }
}

} // namespace gpui
