#pragma once

#include <memory>
#include <string>

namespace algos::hymde::record_match_indexes::orders {
// A bounded non-strict total order.
template <typename CompType>
class TotalOrder {
public:
    using Type = CompType;

    static bool IsValid(CompType const&) {
        return true;
    }

    virtual std::string ToString() const = 0;

    virtual bool AreInOrder(CompType const& res1, CompType const& res2) const = 0;
    // Least and greatest elements must be distinct.
    virtual CompType LeastElement() const = 0;
    virtual CompType GreatestElement() const = 0;

    virtual ~TotalOrder() = default;
};

// A non-owning wrapper to use TotalOrder in C++ STL collections.
template <typename CompType>
class OrderCompareWrapper {
    TotalOrder<CompType> const* order_ptr_;

public:
    OrderCompareWrapper(TotalOrder<CompType> const* order_ptr) : order_ptr_(order_ptr) {}

    bool operator()(CompType const& left, CompType const& right) const {
        return left != right && order_ptr_->AreInOrder(left, right);
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
