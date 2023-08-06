#include <stdexcept>

template <typename K, typename V>
algos::fastod::CacheWithLimit<K, V>::CacheWithLimit(size_t max_size) noexcept : max_size_(max_size) {};

template <typename K, typename V>
bool algos::fastod::CacheWithLimit<K, V>::Contains(const K& key) const noexcept {
    return entries_.find(key) != entries_.end();
}

template <typename K, typename V>
const V& algos::fastod::CacheWithLimit<K, V>::Get(const K& key) const noexcept {
    return entries_.at(key);
}

template <typename K, typename V>
void algos::fastod::CacheWithLimit<K, V>::Set(const K& key, const V& value) {
    if (Contains(key)) {
        throw std::logic_error("Updaing a cache entry is not supported");
    }

    if (keys_in_order_.size() >= max_size_) {
        auto oldest_element_key = keys_in_order_.front();
        keys_in_order_.pop();
        entries_.erase(oldest_element_key);
    }

    keys_in_order_.push(key);
    entries_.emplace(key, value);
}
