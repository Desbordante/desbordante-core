#pragma once

#include <algorithm>
#include <concepts>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "dc/FastADC/model/predicate.h"

namespace algos::fastadc {

/**
 * @brief Manager of unique indices for a collection of objects.
 *
 * This template class assigns a unique index to each distinct object added to it and maintains
 * a bidirectional mapping between objects and their indices.
 */
template <std::totally_ordered<> T>
class IndexProvider {
private:
    size_t next_index_ = 0;
    std::vector<T> objects_;
    std::unordered_map<T, size_t> indexes_;

    void Clear() {
        objects_.clear();
        indexes_.clear();
        next_index_ = 0;
    }

public:
    IndexProvider() = default;
    IndexProvider(IndexProvider const&) = delete;
    IndexProvider& operator=(IndexProvider const&) = delete;
    IndexProvider(IndexProvider&&) = default;
    IndexProvider& operator=(IndexProvider&&) = default;

    size_t GetIndex(T object);

    void AddAll(std::vector<T> const& objects);

    T GetObject(size_t index) const;

    size_t Size() const;

    void Sort();
};

using PredicateIndexProvider = IndexProvider<PredicatePtr>;

// FIXME: I don't like it... looks really wrong.
// I guess something like in FastOD should be done to hash all columns of the table.
// But it gets the job done, so it's fine by now
using IntIndexProvider = IndexProvider<int64_t>;
using DoubleIndexProvider = IndexProvider<double>;
using StringIndexProvider = IndexProvider<std::string>;

}  // namespace algos::fastadc
