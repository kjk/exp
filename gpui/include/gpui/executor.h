#pragma once

/// C++ port of GPUI's executor module.
/// Provides an async task executor integrated with the UI event loop.

#include <gpui/scheduler.h>
#include <functional>
#include <future>
#include <memory>
#include <vector>

namespace gpui {

/// The executor manages async tasks within the GPUI application.
/// It integrates with the platform event loop to dispatch main-thread work.
class Executor {
public:
    Executor() : scheduler_(std::make_unique<Scheduler>()) {}
    explicit Executor(std::size_t num_threads)
        : scheduler_(std::make_unique<Scheduler>(num_threads)) {}

    ~Executor() = default;
    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    /// Spawn a background task and return a handle to its result.
    template <typename F, typename R = std::invoke_result_t<F>>
    TaskHandle<R> spawn(F&& func) {
        return scheduler_->spawn(std::forward<F>(func));
    }

    /// Spawn a high-priority background task.
    template <typename F, typename R = std::invoke_result_t<F>>
    TaskHandle<R> spawn_urgent(F&& func) {
        return scheduler_->spawn(TaskPriority::High, std::forward<F>(func));
    }

    /// Spawn a low-priority background task.
    template <typename F, typename R = std::invoke_result_t<F>>
    TaskHandle<R> spawn_idle(F&& func) {
        return scheduler_->spawn(TaskPriority::Low, std::forward<F>(func));
    }

    /// Dispatch a callback to run on the main thread.
    void dispatch_on_main(std::function<void()> callback) {
        scheduler_->dispatch_main(std::move(callback));
    }

    /// Poll pending main-thread tasks. Must be called from the event loop.
    void poll() {
        scheduler_->poll_main();
    }

    /// Create a timer that fires after a delay.
    std::shared_ptr<Timer> set_timeout(
        std::chrono::milliseconds delay,
        std::function<void()> callback) {
        auto timer = std::make_shared<Timer>(delay, std::move(callback), false);
        std::lock_guard lock(timers_mutex_);
        timers_.push_back(timer);
        return timer;
    }

    /// Create a repeating timer.
    std::shared_ptr<Timer> set_interval(
        std::chrono::milliseconds interval,
        std::function<void()> callback) {
        auto timer = std::make_shared<Timer>(interval, std::move(callback), true);
        std::lock_guard lock(timers_mutex_);
        timers_.push_back(timer);
        return timer;
    }

    /// Poll all active timers. Call from the event loop.
    void poll_timers() {
        std::lock_guard lock(timers_mutex_);
        timers_.erase(
            std::remove_if(timers_.begin(), timers_.end(),
                [](const std::shared_ptr<Timer>& t) {
                    t->poll();
                    return !t->is_active();
                }),
            timers_.end());
    }

private:
    std::unique_ptr<Scheduler> scheduler_;
    std::mutex timers_mutex_;
    std::vector<std::shared_ptr<Timer>> timers_;
};

} // namespace gpui
