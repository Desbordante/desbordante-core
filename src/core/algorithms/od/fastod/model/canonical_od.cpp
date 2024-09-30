#include "canonical_od.h"

#include <sstream>

#include "algorithms/od/fastod/partitions/complex_stripped_partition.h"

namespace algos::fastod {

template <od::Ordering Ordering>
CanonicalOD<Ordering>::CanonicalOD(AttributeSet const& context, model::ColumnIndex left,
                                   model::ColumnIndex right)
    : context_(std::move(context)), ap_(left, right) {}

template <od::Ordering Ordering>
bool CanonicalOD<Ordering>::IsValid(DataFrame const& data, PartitionCache& cache) const {
    return !(cache.GetStrippedPartition(context_, data).Swap<Ordering>(ap_.left, ap_.right));
}

template <od::Ordering Ordering>
od::RemovalSetAsVec CanonicalOD<Ordering>::CalculateRemovalSet(DataFrame const& data,
                                                               PartitionCache& cache) const {
    return cache.GetStrippedPartition(context_, data)
            .CalculateSwapRemovalSet<Ordering>(ap_.left, ap_.right);
}

template <od::Ordering Ordering>
std::string CanonicalOD<Ordering>::ToString() const {
    std::stringstream result;

    result << context_.ToString() << " : " << ap_.left + 1
           << ((Ordering == +od::Ordering::ascending) ? "<=" : ">=") << " ~ " << ap_.right + 1
           << "<=";

    return result.str();
}

SimpleCanonicalOD::SimpleCanonicalOD() : right_(0) {}

SimpleCanonicalOD::SimpleCanonicalOD(AttributeSet const& context, model::ColumnIndex right)
    : context_(context), right_(right) {}

bool SimpleCanonicalOD::IsValid(DataFrame const& data, PartitionCache& cache) const {
    return !(cache.GetStrippedPartition(context_, data).Split(right_));
}

od::RemovalSetAsVec SimpleCanonicalOD::CalculateRemovalSet(DataFrame const& data,
                                                           PartitionCache& cache) const {
    return cache.GetStrippedPartition(context_, data).CalculateSplitRemovalSet(right_);
}

std::string SimpleCanonicalOD::ToString() const {
    std::stringstream result;
    result << context_.ToString() << " : [] -> " << right_ + 1 << "<=";

    return result.str();
}

bool operator==(CanonicalOD<od::Ordering::ascending> const& x,
                CanonicalOD<od::Ordering::ascending> const& y) {
    return x.context_ == y.context_ && x.ap_ == y.ap_;
}

bool operator!=(CanonicalOD<od::Ordering::ascending> const& x,
                CanonicalOD<od::Ordering::ascending> const& y) {
    return !(x == y);
}

bool operator<(CanonicalOD<od::Ordering::ascending> const& x,
               CanonicalOD<od::Ordering::ascending> const& y) {
    if (x.ap_ != y.ap_) {
        return x.ap_ < y.ap_;
    }

    return x.context_ < y.context_;
}

bool operator==(CanonicalOD<od::Ordering::descending> const& x,
                CanonicalOD<od::Ordering::descending> const& y) {
    return x.context_ == y.context_ && x.ap_ == y.ap_;
}

bool operator!=(CanonicalOD<od::Ordering::descending> const& x,
                CanonicalOD<od::Ordering::descending> const& y) {
    return !(x == y);
}

bool operator<(CanonicalOD<od::Ordering::descending> const& x,
               CanonicalOD<od::Ordering::descending> const& y) {
    if (x.ap_ != y.ap_) {
        return x.ap_ < y.ap_;
    }

    return x.context_ < y.context_;
}

bool operator==(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y) {
    return x.context_ == y.context_ && x.right_ == y.right_;
}

bool operator!=(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y) {
    return !(x == y);
}

bool operator<(SimpleCanonicalOD const& x, SimpleCanonicalOD const& y) {
    if (x.right_ != y.right_) {
        return x.right_ < y.right_;
    }

    return x.context_ < y.context_;
}

template class CanonicalOD<od::Ordering::ascending>;
template class CanonicalOD<od::Ordering::descending>;

}  // namespace algos::fastod
