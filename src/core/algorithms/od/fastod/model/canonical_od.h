#pragma once

#include <memory>

#include "algorithms/od/fastod/hashing/hashing.h"
#include "algorithms/od/fastod/od_ordering.h"
#include "algorithms/od/fastod/storage/partition_cache.h"
#include "attribute_pair.h"
#include "error/type.h"

namespace algos::fastod {

template <od::Ordering Ordering>
class CanonicalOD {
private:
    AttributeSet context_;
    AttributePair ap_;

public:
    CanonicalOD() noexcept = default;
    CanonicalOD(AttributeSet const& context, model::ColumnIndex left, model::ColumnIndex right);

    bool IsValid(DataFrame const& data, PartitionCache& cache, config::ErrorType error = 0) const;
    od::RemovalSetAsVec CalculateRemovalSet(DataFrame const& data, PartitionCache& cache) const;
    std::string ToString() const;

    AttributeSet const& GetContext() const noexcept {
        return context_;
    }

    AttributePair const& GetAttributePair() const {
        return ap_;
    }

    model::ColumnIndex GetLeftColumn() const noexcept {
        return ap_.left;
    }

    model::ColumnIndex GetRightColumn() const noexcept {
        return ap_.right;
    }

    constexpr static auto kName = "OC";

    friend bool operator==(CanonicalOD<od::Ordering::kAscending> const& De,
                           CanonicalOD<od::Ordering::kAscending> const& y);
    friend bool operator!=(CanonicalOD<od::Ordering::kAscending> const& x,
                           CanonicalOD<od::Ordering::kAscending> const& y);
    friend bool operator<(CanonicalOD<od::Ordering::kAscending> const& x,
                          CanonicalOD<od::Ordering::kAscending> const& y);
    friend bool operator==(CanonicalOD<od::Ordering::kDescending> const& x,
                           CanonicalOD<od::Ordering::kDescending> const& y);
    friend bool operator!=(CanonicalOD<od::Ordering::kDescending> const& x,
                           CanonicalOD<od::Ordering::kDescending> const& y);
    friend bool operator<(CanonicalOD<od::Ordering::kDescending> const& x,
                          CanonicalOD<od::Ordering::kDescending> const& y);

    friend struct std::hash<CanonicalOD<Ordering>>;
};

using AscCanonicalOD = CanonicalOD<od::Ordering::kAscending>;
using DescCanonicalOD = CanonicalOD<od::Ordering::kDescending>;

class SimpleCanonicalOD {
private:
    AttributeSet context_;
    model::ColumnIndex right_;

public:
    SimpleCanonicalOD();
    SimpleCanonicalOD(AttributeSet const& context, model::ColumnIndex right);

    bool IsValid(DataFrame const& data, PartitionCache& cache, config::ErrorType error = 0) const;
    od::RemovalSetAsVec CalculateRemovalSet(DataFrame const& data, PartitionCache& cache) const;
    std::string ToString() const;

    AttributeSet const& GetContext() const noexcept {
        return context_;
    }

    model::ColumnIndex GetRight() const noexcept {
        return right_;
    }

    constexpr static auto kName = "OFD";

    friend bool operator==(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y);
    friend bool operator!=(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y);
    friend bool operator<(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y);

    friend struct std::hash<SimpleCanonicalOD>;
};

}  // namespace algos::fastod

template <algos::od::Ordering Ordering>
struct std::hash<algos::fastod::CanonicalOD<Ordering>> {
    size_t operator()(algos::fastod::CanonicalOD<Ordering> const& od) const noexcept {
        size_t const context_hash = std::hash<algos::fastod::AttributeSet>{}(od.context_);
        size_t const ap_hash = std::hash<algos::fastod::AttributePair>{}(od.ap_);

        return algos::fastod::hashing::CombineHashes(context_hash, ap_hash);
    }
};

template <>
struct std::hash<algos::fastod::SimpleCanonicalOD> {
    size_t operator()(algos::fastod::SimpleCanonicalOD const& od) const noexcept {
        size_t const context_hash = std::hash<algos::fastod::AttributeSet>{}(od.context_);
        size_t const right_hash = std::hash<model::ColumnIndex>{}(od.right_);

        return algos::fastod::hashing::CombineHashes(context_hash, right_hash);
    }
};
