#pragma once

#include <cstddef>
#include <memory>

#include "entry.h"

namespace algos::cfdfinder {

class NegativeConstantEntry final : public Entry {
private:
    size_t constant_;

public:
    explicit NegativeConstantEntry(size_t constant) : constant_(constant) {}

    inline bool Matches(size_t value) const override final {
        return constant_ != value;
    }

    bool operator==(Entry const& other) const override {
        if (other.GetType() != EntryType::kNegativeConstant) return false;
        return constant_ == static_cast<NegativeConstantEntry const&>(other).constant_;
    }

    size_t Hash() const override {
        return std::hash<size_t>{}(constant_) * 31;
    }

    size_t GetConstant() const {
        return constant_;
    }

    EntryType GetType() const override final {
        return EntryType::kNegativeConstant;
    }
};
}  // namespace algos::cfdfinder