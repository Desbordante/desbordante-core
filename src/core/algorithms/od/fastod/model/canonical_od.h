#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <variant>

#include "algorithms/od/fastod/hashing/hashing.h"
#include "algorithms/od/fastod/storage/partition_cache.h"
#include "attribute_pair.h"
#include "od/fastod/model/attribute_set.h"
#include "table/column_index.h"

namespace algos {
namespace fastod {
class DataFrame;
class PartitionCache;
}  // namespace fastod
}  // namespace algos

namespace algos::fastod {

template <bool Ascending>
class CanonicalOD {
private:
    AttributeSet context_;
    AttributePair ap_;

public:
    CanonicalOD() noexcept = default;
    CanonicalOD(AttributeSet const& context, model::ColumnIndex left, model::ColumnIndex right);

    bool IsValid(std::shared_ptr<DataFrame> data, PartitionCache& cache) const;
    std::string ToString() const;

    AttributeSet const& GetContext() const {
        return context_;
    }

    AttributePair const& GetAttributePair() const {
        return ap_;
    }

    friend bool operator==(CanonicalOD<true> const& x, CanonicalOD<true> const& y);
    friend bool operator!=(CanonicalOD<true> const& x, CanonicalOD<true> const& y);
    friend bool operator<(CanonicalOD<true> const& x, CanonicalOD<true> const& y);
    friend bool operator==(CanonicalOD<false> const& x, CanonicalOD<false> const& y);
    friend bool operator!=(CanonicalOD<false> const& x, CanonicalOD<false> const& y);
    friend bool operator<(CanonicalOD<false> const& x, CanonicalOD<false> const& y);

    friend struct std::hash<CanonicalOD<Ascending>>;
};

using AscCanonicalOD = CanonicalOD<true>;
using DescCanonicalOD = CanonicalOD<false>;

class SimpleCanonicalOD {
private:
    AttributeSet context_;
    model::ColumnIndex right_;

public:
    SimpleCanonicalOD();
    SimpleCanonicalOD(AttributeSet const& context, model::ColumnIndex right);

    bool IsValid(std::shared_ptr<DataFrame> data, PartitionCache& cache) const;
    std::string ToString() const;

    AttributeSet const& GetContext() const {
        return context_;
    }

    model::ColumnIndex GetRight() const {
        return right_;
    }

    friend bool operator==(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y);
    friend bool operator!=(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y);
    friend bool operator<(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y);

    friend struct std::hash<SimpleCanonicalOD>;
};

}  // namespace algos::fastod

namespace std {

template <bool Ascending>
struct hash<algos::fastod::CanonicalOD<Ascending>> {
    size_t operator()(algos::fastod::CanonicalOD<Ascending> const& od) const noexcept {
        size_t const context_hash = hash<algos::fastod::AttributeSet>{}(od.context_);
        size_t const ap_hash = hash<algos::fastod::AttributePair>{}(od.ap_);

        return algos::fastod::hashing::CombineHashes(context_hash, ap_hash);
    }
};

template <>
struct hash<algos::fastod::SimpleCanonicalOD> {
    size_t operator()(algos::fastod::SimpleCanonicalOD const& od) const noexcept {
        size_t const context_hash = hash<algos::fastod::AttributeSet>{}(od.context_);
        size_t const right_hash = hash<model::ColumnIndex>{}(od.right_);

        return algos::fastod::hashing::CombineHashes(context_hash, right_hash);
    }
};

}  // namespace std
