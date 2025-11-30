#pragma once

#include <cassert>

#include <boost/math/special_functions/next.hpp>
#include <boost/math/special_functions/relative_difference.hpp>

#include "core/model/types/numeric_type.h"

namespace model {

class DoubleType final : public NumericType<Double> {
public:
    DoubleType() noexcept : NumericType<Double>(TypeId::kDouble) {}

    static unsigned int const kDefaultEpsCount = 5;

    CompareResult Compare(std::byte const* l, std::byte const* r) const final {
        return CompareEPS(l, r, kDefaultEpsCount);
    }

    static CompareResult CompareEPS(std::byte const* l, std::byte const* r,
                                    unsigned int eps_count) {
        Double l_val = GetValue(l);
        Double r_val = GetValue(r);

        if (boost::math::relative_difference(l_val, r_val) <
            eps_count * std::numeric_limits<model::Double>::epsilon()) {
            return CompareResult::kEqual;
        }

        if (l_val < r_val) {
            return CompareResult::kLess;
        }

        return CompareResult::kGreater;
    }

    [[nodiscard]] std::unique_ptr<Type> CloneType() const override {
        return std::make_unique<DoubleType>();
    }

    static std::byte* MakeFrom(std::byte const* data, Type const& type) {
        std::byte* dest = DoubleType().Allocate();
        MakeFrom(data, type, dest);

        return dest;
    }

    // Unlike MakeFrom above does put a converted value in the already allocated memory
    static void MakeFrom(std::byte const* data, Type const& type, std::byte* dest) {
        switch (type.GetTypeId()) {
            case TypeId::kDouble:
                GetValue(dest) = GetValue(data);
                break;
            case TypeId::kInt:
                GetValue(dest) = Type::GetValue<Int>(data);
                break;
            default:
                assert(false);
                __builtin_unreachable();
        }
    }
};

}  // namespace model
