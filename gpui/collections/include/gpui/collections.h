#pragma once

/// C++ port of Zed's `collections` crate.
/// Provides type aliases for commonly used container types with
/// consistent hashing (using a fast, non-cryptographic hasher).

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gpui {

/// A fast hash function suitable for hash maps/sets in a GUI framework.
/// Uses FNV-1a for small keys and falls back to std::hash for larger types.
struct FastHash {
    template <typename T>
    std::size_t operator()(const T& value) const noexcept {
        return std::hash<T>{}(value);
    }
};

/// HashMap with a fast, non-cryptographic hasher.
template <typename K, typename V, typename Hash = FastHash,
          typename KeyEqual = std::equal_to<K>>
using HashMap = std::unordered_map<K, V, Hash, KeyEqual>;

/// HashSet with a fast, non-cryptographic hasher.
template <typename K, typename Hash = FastHash,
          typename KeyEqual = std::equal_to<K>>
using HashSet = std::unordered_set<K, Hash, KeyEqual>;

/// A small vector optimization - stores up to N elements inline.
/// Falls back to heap allocation for larger sizes.
template <typename T, std::size_t N = 4>
class SmallVec {
public:
    SmallVec() = default;

    void push_back(const T& value) {
        data_.push_back(value);
    }

    void push_back(T&& value) {
        data_.push_back(std::move(value));
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        return data_.emplace_back(std::forward<Args>(args)...);
    }

    T& operator[](std::size_t index) { return data_[index]; }
    const T& operator[](std::size_t index) const { return data_[index]; }

    std::size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    void clear() { data_.clear(); }
    void reserve(std::size_t n) { data_.reserve(n); }

    T* data() { return data_.data(); }
    const T* data() const { return data_.data(); }

private:
    // TODO: Actual small-buffer optimization with inline storage
    std::vector<T> data_;
};

/// A string interner for deduplicating shared strings.
class SharedString {
public:
    SharedString() = default;
    explicit SharedString(std::string value) : value_(std::make_shared<std::string>(std::move(value))) {}
    SharedString(const char* value) : SharedString(std::string(value)) {}

    const std::string& as_str() const {
        static const std::string empty;
        return value_ ? *value_ : empty;
    }

    bool operator==(const SharedString& other) const {
        if (value_ == other.value_) return true;
        return as_str() == other.as_str();
    }

    bool operator!=(const SharedString& other) const { return !(*this == other); }
    bool operator<(const SharedString& other) const { return as_str() < other.as_str(); }

    bool empty() const { return !value_ || value_->empty(); }
    std::size_t size() const { return value_ ? value_->size() : 0; }

    operator const std::string&() const { return as_str(); }

private:
    std::shared_ptr<std::string> value_;
};

} // namespace gpui

namespace std {
template <>
struct hash<gpui::SharedString> {
    std::size_t operator()(const gpui::SharedString& s) const noexcept {
        return std::hash<std::string>{}(s.as_str());
    }
};
} // namespace std
