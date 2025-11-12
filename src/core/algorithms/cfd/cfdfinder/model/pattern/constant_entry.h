#pragma once

#include "entry.h"

namespace algos::cfdfinder {

class ConstantEntry final : public Entry {
private:
    size_t constant_;

public:
    explicit ConstantEntry(size_t constant) : constant_(constant) {}

    inline bool Matches(size_t value) const override final {
        return constant_ == value;
    }

    bool operator==(Entry const& other) const override final {
        if (other.GetType() != EntryType::kConstant) return false;
        return constant_ == static_cast<ConstantEntry const&>(other).constant_;
    }

    size_t Hash() const override {
        return std::hash<size_t>{}(constant_);
    }

    size_t GetConstant() const {
        return constant_;
    }

    EntryType GetType() const override final {
        return EntryType::kConstant;
    }
};
}  // namespace algos::cfdfinder