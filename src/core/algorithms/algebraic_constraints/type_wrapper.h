#pragma once
#include <memory>
#include <string>

#include "builtin.h"
#include "create_type.h"
#include "date_type.h"
#include "numeric_type.h"

namespace algos {
class TypeWrapper {
public:
    std::unique_ptr<model::INumericType> num_type;
    std::unique_ptr<model::DateType> date_type;

    TypeWrapper() : num_type(nullptr), date_type(nullptr) {}

    bool IsNumeric() const {
        return date_type == nullptr;
    }

    void Set(model::TypeId id) {
        switch (id) {
            case model::TypeId::kDate:
                date_type = model::CreateSpecificType<model::DateType>(id, true);
                num_type =
                        model::CreateSpecificType<model::INumericType>(+model::TypeId::kInt, true);
                break;
            default:
                num_type = model::CreateSpecificType<model::INumericType>(id, true);
                date_type = nullptr;
        }
    }

    explicit TypeWrapper(model::TypeId id) {
        Set(id);
    };

    model::CompareResult NumericCompare(std::byte const* l, std::byte const* r) const {
        return num_type->Compare(l, r);
    }

    [[nodiscard]] std::byte* NumericAllocate(size_t count = 1) const {
        return num_type->Allocate(count);
    }

    void NumericFromStr(std::byte* buf, std::string const& s) const {
        return num_type->ValueFromStr(buf, s);
    }

    double Dist(std::byte const* l, std::byte const* r) const {
        if (IsNumeric()) {
            return num_type->Dist(l, r);
        }
        return date_type->Dist(l, r);
    }

    [[nodiscard]] model::TypeId GetTypeId() const {
        if (IsNumeric()) {
            return num_type->GetTypeId();
        }
        return date_type->GetTypeId();
    }

    [[nodiscard]] model::TypeId GetNumericId() const {
        return num_type->GetTypeId();
    }

    std::string NumericToString(std::byte const* value) const {
        return num_type->ValueToString(value);
    }
};
}  // namespace algos