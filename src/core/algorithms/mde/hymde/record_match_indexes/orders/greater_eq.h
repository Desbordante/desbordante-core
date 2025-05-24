#pragma once

#include <string>

namespace algos::hymde::record_match_indexes::orders {
template <typename Base>
class GreaterEq : public Base {
public:
    using Type = Base::Type;

    std::string ToString() const final {
        return ">=";
    }

    bool AreInOrder(Type const& res1, Type const& res2) const final {
        return res1 >= res2;
    }
};
}  // namespace algos::hymde::record_match_indexes::orders
