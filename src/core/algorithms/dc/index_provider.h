#pragma once

#include <concepts>
#include <unordered_map>
#include <vector>

#include "dc/base_provider.h"
#include "dc/predicate.h"

namespace model {

/**
 * @brief Singleton manager of unique indices for a collection of objects.
 *
 * This template class assigns a unique index to each distinct object added to it and maintains
 * a bidirectional mapping between objects and their indices.
 */
template <std::totally_ordered<> T>
class IndexProvider : public BaseProvider<IndexProvider<T>> {
private:
    size_t next_index_ = 0;
    std::vector<T> objects_;
    std::unordered_map<T, size_t> indexes_;

    friend BaseProvider<IndexProvider<T>>;

    static std::string ClassName() {
        return "IndexProvider";
    }

    static void Clear() {
        BaseProvider<IndexProvider<T>>::instance_->objects_.clear();
        BaseProvider<IndexProvider<T>>::instance_->indexes_.clear();
    }

public:
    size_t GetIndex(T object);

    void AddAll(std::vector<T> const& objects);

    T GetObject(size_t index) const;

    size_t Size() const;

    void Sort();
};

using PredicateIndexProvider = IndexProvider<PredicatePtr>;

}  // namespace model
