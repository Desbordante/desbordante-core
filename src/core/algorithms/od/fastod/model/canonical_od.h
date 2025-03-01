#pragma once

#include <algorithm>   // for sort
#include <cstddef>     // for size_t
#include <functional>  // for hash
#include <memory>      // for shared_ptr
#include <string>      // for string

#include "algorithms/od/fastod/hashing/hashing.h"  // for CombineHashes
#include "attribute_pair.h"                        // for AttributePair, hash
#include "od/fastod/model/attribute_set.h"         // for hash, AttributeSet
#include "table/column_index.h"                    // for ColumnIndex

namespace algos {
namespace fastod {
class DataFrame;
}
}  // namespace algos

namespace algos {
namespace fastod {
class PartitionCache;
}
}  // namespace algos

namespace std {
template <typename>
struct hash;
}

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
