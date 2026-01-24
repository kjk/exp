#pragma once

/// C++ port of Zed's `scheduler` crate.
/// Provides task scheduling primitives for the GPUI event loop.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

namespace gpui {

/// Priority levels for scheduled tasks.
enum class TaskPriority {
    /// Tasks that must run before the next frame.
    High,
    /// Normal priority tasks.
    Normal,
    /// Low priority background tasks.
    Low,
};

/// A handle to a spawned task. Can be used to await or cancel the task.
template <typename T = void>
class TaskHandle {
public:
    TaskHandle() = default;
    explicit TaskHandle(std::shared_future<T> future) : future_(std::move(future)) {}

    /// Block until the task completes and return the result.
    T get() { return future_.get(); }

    /// Check if the task has completed.
    bool is_ready() const {
        return future_.valid() &&
               future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    bool valid() const { return future_.valid(); }

private:
    std::shared_future<T> future_;
};

/// A scheduler that manages a pool of worker threads and a task queue.
class Scheduler {
public:
    explicit Scheduler(std::size_t num_threads = std::thread::hardware_concurrency()) {
        for (std::size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ~Scheduler() {
        {
            std::lock_guard lock(mutex_);
            shutdown_ = true;
        }
        cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    /// Spawn a task with the given priority.
    template <typename F, typename R = std::invoke_result_t<F>>
    TaskHandle<R> spawn(TaskPriority priority, F&& func) {
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(func));
        auto future = task->get_future().share();

        {
            std::lock_guard lock(mutex_);
            auto& queue = queue_for(priority);
            queue.emplace_back([task = std::move(task)]() { (*task)(); });
        }
        cv_.notify_one();

        return TaskHandle<R>(std::move(future));
    }

    /// Spawn a task with normal priority.
    template <typename F, typename R = std::invoke_result_t<F>>
    TaskHandle<R> spawn(F&& func) {
        return spawn(TaskPriority::Normal, std::forward<F>(func));
    }

    /// Run a callback on the main thread (must be polled from the event loop).
    void dispatch_main(std::function<void()> callback) {
        std::lock_guard lock(main_mutex_);
        main_queue_.push_back(std::move(callback));
    }

    /// Poll and execute pending main-thread callbacks. Call from the event loop.
    void poll_main() {
        std::vector<std::function<void()>> tasks;
        {
            std::lock_guard lock(main_mutex_);
            tasks.swap(main_queue_);
        }
        for (auto& task : tasks) {
            task();
        }
    }

private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, [this] {
                    return shutdown_ || !high_.empty() || !normal_.empty() || !low_.empty();
                });
                if (shutdown_ && high_.empty() && normal_.empty() && low_.empty()) {
                    return;
                }
                // Priority ordering
                if (!high_.empty()) {
                    task = std::move(high_.front());
                    high_.pop_front();
                } else if (!normal_.empty()) {
                    task = std::move(normal_.front());
                    normal_.pop_front();
                } else if (!low_.empty()) {
                    task = std::move(low_.front());
                    low_.pop_front();
                }
            }
            if (task) task();
        }
    }

    std::deque<std::function<void()>>& queue_for(TaskPriority p) {
        switch (p) {
            case TaskPriority::High: return high_;
            case TaskPriority::Normal: return normal_;
            case TaskPriority::Low: return low_;
        }
        return normal_;
    }

    std::vector<std::thread> workers_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool shutdown_ = false;

    std::deque<std::function<void()>> high_;
    std::deque<std::function<void()>> normal_;
    std::deque<std::function<void()>> low_;

    std::mutex main_mutex_;
    std::vector<std::function<void()>> main_queue_;
};

/// A timer that fires a callback after a delay.
class Timer {
public:
    using Duration = std::chrono::milliseconds;
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    Timer(Duration delay, std::function<void()> callback, bool repeating = false)
        : delay_(delay)
        , callback_(std::move(callback))
        , repeating_(repeating)
        , next_fire_(Clock::now() + delay)
        , active_(true) {}

    /// Check if the timer should fire and execute if so. Returns true if fired.
    bool poll() {
        if (!active_) return false;
        if (Clock::now() >= next_fire_) {
            callback_();
            if (repeating_) {
                next_fire_ = Clock::now() + delay_;
            } else {
                active_ = false;
            }
            return true;
        }
        return false;
    }

    void cancel() { active_ = false; }
    bool is_active() const { return active_; }

private:
    Duration delay_;
    std::function<void()> callback_;
    bool repeating_;
    TimePoint next_fire_;
    bool active_;
};

} // namespace gpui
