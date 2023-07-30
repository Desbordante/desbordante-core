#include "cache_with_limit.h"
#include <stdexcept>

using namespace algos::fastod;

template <typename K, typename V>
CacheWithLimit<K, V>::CacheWithLimit(size_t max_size) noexcept : max_size_(max_size) {};

template <typename K, typename V>
bool CacheWithLimit<K, V>::Contains(K key) const noexcept {
    return entries_.find(key) != entries_.end();
}

template <typename K, typename V>
V& CacheWithLimit<K, V>::Get(K key) const noexcept {
    return entries_[key];
}

template <typename K, typename V>
void CacheWithLimit<K, V>::Set(K key, V value) {
    if (Contains(key)) {
        throw std::logic_error("Updaing a cache entry is not supported");
    }

    if (keys_in_order_.size() >= max_size_) {
        auto oldest_element_key = keys_in_order_.pop();
        entries_.erase(oldest_element_key);
    }

    keys_in_order_.push(key);
    entries_[key] = value;
}

