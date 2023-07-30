# pragma once

#include <map>
#include <queue>

namespace algos::fastod {

template <typename K, typename V>
class CacheWithLimit {
private:
    std::map<K, V> entries_;
    std::queue<K> keys_in_order_;
    const size_t max_size_;

public:
    CacheWithLimit(size_t max_size) noexcept;
    
    bool Contains(K key) const noexcept;
    V& Get(K key) const noexcept;
    void Set(K key, V value);
};

} // namespace algos::fastod;
