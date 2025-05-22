#pragma once

#include <string>

#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"
#include "config/exceptions.h"

namespace algos::hymde::record_match_indexes::orders {
template <typename Base, typename Comparer>
class Custom final : public Base {
    using Type = Base::Type;

    Comparer comparer_;
    std::string display_name_;
    Type least_;
    Type greatest_;

public:
    template <typename ComparerConv>
    Custom(ComparerConv comparer_conv, Type least, Type greatest,
           std::string display_name = "(custom order)")
        : comparer_(std::move(comparer_conv)),
          display_name_(std::move(display_name)),
          least_(std::move(least)),
          greatest_(std::move(greatest)) {
        if (!Base::IsValid(least_))
            throw config::ConfigurationError("Invalid least element for order " + display_name_);
        if (!Base::IsValid(greatest_))
            throw config::ConfigurationError("Invalid greatest element for order " + display_name_);
        if (!AreInOrder(least_, greatest_))
            throw config::ConfigurationError("Error for order " + display_name_ +
                                             ": least element must be less than greatest element!");
        if (AreInOrder(greatest_, least_))
            throw config::ConfigurationError(
                    "Error for order " + display_name_ +
                    ": least element must be distinct from greatest element!");
    }

    bool AreInOrder(Type const& res1, Type const& res2) const final {
        return comparer_(res1, res2);
    }

    std::string ToString() const {
        return display_name_;
    }

    Type LeastElement() const final {
        return least_;
    }

    Type GreatestElement() const final {
        return greatest_;
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
