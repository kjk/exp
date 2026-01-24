#pragma once

/// C++ port of Zed's `util` crate.
/// General-purpose utilities used throughout the GPUI framework.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

namespace gpui {

/// Result type - simplified version of Rust's Result<T, E>.
/// Uses std::expected in C++23, but for C++20 we use a simple wrapper.
template <typename T, typename E = std::string>
class Result {
public:
    static Result ok(T value) { return Result(std::move(value), {}); }
    static Result err(E error) { return Result({}, std::move(error)); }

    bool is_ok() const { return has_value_; }
    bool is_err() const { return !has_value_; }

    T& value() { return value_; }
    const T& value() const { return value_; }
    E& error() { return error_; }
    const E& error() const { return error_; }

    T value_or(T default_val) const {
        return has_value_ ? value_ : std::move(default_val);
    }

private:
    Result(T value, E error)
        : value_(std::move(value)), error_(std::move(error)), has_value_(true) {}
    Result(std::nullopt_t, E error)
        : error_(std::move(error)), has_value_(false) {}

    T value_{};
    E error_{};
    bool has_value_ = false;
};

/// Truncate a string to the given max byte length, respecting UTF-8 boundaries.
inline std::string truncate_string(const std::string& s, std::size_t max_len) {
    if (s.size() <= max_len) return s;
    // Find the last valid UTF-8 boundary before max_len
    std::size_t end = max_len;
    while (end > 0 && (static_cast<unsigned char>(s[end]) & 0xC0) == 0x80) {
        --end;
    }
    return s.substr(0, end);
}

/// Numeric extension: clamp a value to a range.
template <typename T>
constexpr T clamp(T value, T min_val, T max_val) {
    return std::max(min_val, std::min(value, max_val));
}

/// Linear interpolation between two values.
template <typename T>
constexpr T lerp(T a, T b, float t) {
    return static_cast<T>(a + (b - a) * t);
}

/// Check if a floating-point value is approximately equal to another.
template <typename T>
constexpr bool approx_eq(T a, T b, T epsilon = static_cast<T>(1e-6)) {
    return std::abs(a - b) < epsilon;
}

/// A monotonically increasing ID generator.
class IdGenerator {
public:
    uint64_t next() { return next_id_++; }

private:
    uint64_t next_id_ = 1;
};

/// Type-erased callback (similar to Rust's Box<dyn FnOnce()>).
using Callback = std::function<void()>;

/// A debouncer that delays execution until a quiet period.
class Debouncer {
public:
    using Duration = std::chrono::milliseconds;
    using Clock = std::chrono::steady_clock;

    explicit Debouncer(Duration delay) : delay_(delay) {}

    /// Schedule a callback. Previous pending callback is cancelled.
    void call(Callback cb) {
        pending_ = std::move(cb);
        deadline_ = Clock::now() + delay_;
    }

    /// Check if the deadline has passed and fire if so.
    bool poll() {
        if (pending_ && Clock::now() >= deadline_) {
            auto cb = std::move(pending_);
            pending_ = nullptr;
            cb();
            return true;
        }
        return false;
    }

    void cancel() { pending_ = nullptr; }

private:
    Duration delay_;
    Callback pending_;
    Clock::time_point deadline_;
};

/// Arc-like shared ownership (just an alias for shared_ptr).
template <typename T>
using Arc = std::shared_ptr<T>;

/// Create a shared pointer (equivalent to Arc::new in Rust).
template <typename T, typename... Args>
Arc<T> make_arc(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

/// Box-like unique ownership (just an alias for unique_ptr).
template <typename T>
using Box = std::unique_ptr<T>;

/// Create a unique pointer (equivalent to Box::new in Rust).
template <typename T, typename... Args>
Box<T> make_box(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

} // namespace gpui
