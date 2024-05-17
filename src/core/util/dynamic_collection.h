#pragma once

#include <mutex>
#include <unordered_set>
#include <vector>

namespace util {

// Represents version Collection with thread-safe deletions for dynamic algorithms
template <typename T>
class DynamicCollection {
private:
    std::unordered_multiset<T> collection_{};
    std::mutex mutable mutex_;

public:
    void Add(T&& primitive) {
        std::scoped_lock lock(mutex_);
        collection_.insert(std::move(primitive));
    }

    template <typename... Args>
    void Add(Args&&... args) {
        std::scoped_lock lock(mutex_);
        collection_.emplace(std::forward<Args>(args)...);
    }

    void Clear() noexcept {
        std::scoped_lock lock(mutex_);
        collection_.clear();
    }

    bool Contains(const T& primitive) noexcept {
        std::scoped_lock lock(mutex_);
        auto it = collection_.find(primitive);
        return it != collection_.end();
    }

    T Erase(const T& primitive) {
        std::scoped_lock lock(mutex_);
        auto it = collection_.find(primitive);
        T res;
        if (it != collection_.end()) {
            res = *it;
            collection_.erase(it);
            return res;
        }
        return res;
    }

    size_t Size() const noexcept {
        std::scoped_lock lock(mutex_);
        return collection_.size();
    }

    std::vector<std::string> AsStringVector() const noexcept {
        std::vector<std::string> result_{};
        for (const T& item : collection_) {
            result_.emplace_back(std::string(item));
        }
        return result_;
    }

    const std::unordered_multiset<T>& AsUnorderedMultiset() const noexcept {
        return collection_;
    }

    std::unordered_multiset<T>& AsUnorderedMultiset() noexcept {
        return collection_;
    }
};

}  // namespace util
