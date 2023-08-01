#include <sstream>

#include "canonical_od.h"
#include "predicates/OperatorType.h"
#include "predicates/SingleAttributePredicate.h"
#include "stripped_partition.h"

using namespace algos::fastod;

int CanonicalOD::split_check_count_ = 0;
int CanonicalOD::swap_check_count_ = 0;

CanonicalOD::CanonicalOD(const AttributeSet& context, const SingleAttributePredicate& left, int right) noexcept : context_(context), left_(left), right_(right) {}

CanonicalOD::CanonicalOD(const AttributeSet& context, int right) noexcept : context_(context), left_({}), right_(right) {}

bool CanonicalOD::IsValid(const DataFrame& data, double error_rate_threshold) const noexcept {
    auto sp = StrippedPartition::GetStrippedPartition(context_, data);

    if (error_rate_threshold == -1) {
        if (!left_.has_value()) {
            split_check_count_++;

            return !sp.Split(right_);
        }

        swap_check_count_++;

        return !sp.Swap(left_.value(), right_);
    }

    long violation_count;

    if (!left_.has_value()) {
        violation_count = sp.SplitRemoveCount(right_);
    } else {
        violation_count = sp.SwapRemoveCount(left_.value(), right_);
    }

    auto error_rate = (double) violation_count / data.GetTupleCount();

    return error_rate < error_rate_threshold;
}

std::string CanonicalOD::ToString() const noexcept {
    std::stringstream ss;

    ss << context_.ToString() << " : ";

    if (left_.has_value()) {
        ss << left_.value().ToString() << " ~ ";
    } else {
        ss << "[] -> ";
    }

    ss << right_ + 1 << "<=";

    return ss.str();
}

namespace algos::fastod {

bool operator==(CanonicalOD const& x, CanonicalOD const& y) {
    return x.context_ == y.context_ && x.left_ == y.left_ && x.right_ == y.right_;
}

// TODO: Check whether x and y should be swapped
bool operator<(CanonicalOD const& x, CanonicalOD const& y) {
    const int attribute_count_difference = x.context_.GetAttributeCount() - y.context_.GetAttributeCount();
    if (attribute_count_difference != 0) {
        return attribute_count_difference < 0;
    }

    const int context_value_difference = x.context_.GetValue() - y.context_.GetValue();
    if (context_value_difference != 0) {
        return context_value_difference < 0;
    }

    const int right_difference = x.right_ - y.right_;
    if (right_difference != 0) {
        return right_difference < 0;
    }

    if (!x.left_.has_value()) {
        return false;
    }

    auto left_difference = x.left_.value().GetAttribute() - y.left_.value().GetAttribute();
    if (left_difference != 0) {
        return left_difference < 0;
    }

    if (x.left_.value().GetOperator() == y.left_.value().GetOperator()) {
        return false;
    }

    if (x.left_.value().GetOperator().GetType() == OperatorType::LessOrEqual) {
        return true;    //TODO: Make sure -1 in Java code corrensponds to true
    }

    return false;
}

} // namespace algos::fastod
