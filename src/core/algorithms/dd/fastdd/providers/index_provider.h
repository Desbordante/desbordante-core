#pragma once

#include <algorithm>
#include <cstddef>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace algos::dd {

/**
 * @brief Manager of unique indices for a collection of objects.
 *
 * This template class assigns a unique index to each distinct object added to it and maintains
 * a bidirectional mapping between objects and their indices.
 */
template <typename T, typename Hash = std::hash<T>, typename KeyEqual = std::equal_to<T>>
class IndexProvider {
private:
    std::size_t next_index_ = 0;
    std::vector<T> objects_;
    std::unordered_map<T, std::size_t, Hash, KeyEqual> indexes_;

public:
    IndexProvider() = default;

    IndexProvider(Hash const& hash, KeyEqual const& key_equal) : indexes_(10, hash, key_equal) {};

    IndexProvider(IndexProvider const&) = delete;
    IndexProvider& operator=(IndexProvider const&) = delete;
    IndexProvider(IndexProvider&&) = default;
    IndexProvider& operator=(IndexProvider&&) = default;

    std::size_t GetIndex(T const& object) {
        auto it = indexes_.find(object);
        if (it == indexes_.end()) {
            indexes_[object] = next_index_;
            objects_.push_back(object);
            return next_index_++;
        }
        return it->second;
    }

    void AddAll(std::vector<T> const& objects) {
        for (auto const& object : objects) GetIndex(object);
    }

    T GetObject(size_t index) const {
        return objects_.at(index);
    }

    std::size_t Size() const {
        return objects_.size();
    }

    template <typename Compare>
    void Sort(Compare comp_func) {
        std::ranges::sort(objects_, comp_func);

        for (std::size_t i = 0; i < objects_.size(); ++i) indexes_[objects_[i]] = i;
    }

    void Clear() {
        objects_.clear();
        indexes_.clear();
        next_index_ = 0;
    }
};

}  // namespace algos::dd
