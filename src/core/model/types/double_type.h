#pragma once

#include <cassert>

#include "numeric_type.h"

namespace model {

class DoubleType final : public NumericType<Double> {
public:
    DoubleType() noexcept : NumericType<Double>(TypeId::kDouble) {}

    CompareResult Compare(std::byte const* l, std::byte const* r) const final {
        Double l_val = GetValue(l);
        Double r_val = GetValue(r);

        if (std::abs(l_val - r_val) < std::numeric_limits<Double>::epsilon()) {
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

    static std::byte* MakeFrom(const std::byte* data, const Type& type) {
        std::byte* dest = DoubleType().Allocate();
        MakeFrom(data, type, dest);

        return dest;
    }

    // Unlike MakeFrom above does put a converted value in the already allocated memory
    static void MakeFrom(const std::byte* data, const Type& type, std::byte* dest) {
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
