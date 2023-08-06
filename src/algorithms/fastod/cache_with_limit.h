#pragma once

#include <map>
#include <queue>
#include <cstddef>

namespace algos::fastod {

template <typename K, typename V>
class CacheWithLimit {
private:
    std::map<K, V> entries_;
    std::queue<K> keys_in_order_;
    const std::size_t max_size_;

public:
    CacheWithLimit(std::size_t max_size) noexcept;
    
    bool Contains(const K& key) const noexcept;
    const V& Get(const K& key) const noexcept;
    void Set(const K& key, const V& value);
};

} // namespace algos::fastod;

#include "cache_with_limit.tpp"
