#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>

namespace algos::rfd::util {

template <typename K, typename V>
class LRUCache {
    struct Entry {
        V value;
        typename std::list<K>::iterator it;
    };

    std::unordered_map<K, Entry> map_;
    std::list<K> list_;
    std::size_t max_size_;

public:
    explicit LRUCache(std::size_t max_size) noexcept : max_size_(max_size) {}

    std::optional<V> get(K const& key) {
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        list_.splice(list_.end(), list_, it->second.it);
        return it->second.value;
    }

    void put(K const& key, V const& value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second.value = value;
            list_.splice(list_.end(), list_, it->second.it);
            return;
        }
        if (map_.size() >= max_size_) {
            K lru_key = list_.front();
            list_.pop_front();
            map_.erase(lru_key);
        }
        list_.push_back(key);
        map_[key] = {value, std::prev(list_.end())};
    }

    void clear() noexcept {
        map_.clear();
        list_.clear();
    }
};

}  // namespace algos::rfd::util
