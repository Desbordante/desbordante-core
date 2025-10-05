#pragma once

#include "entry.h"

namespace algos::cfdfinder {

class VariableEntry final : public Entry {
public:
    inline bool Matches([[maybe_unused]] size_t value) const override final {
        return true;
    }

    bool operator==(Entry const& other) const override final {
        return other.GetType() == EntryType::kVariable;
    }

    EntryType GetType() const override final {
        return EntryType::kVariable;
    }

    size_t Hash() const override {
        return 0x9e3779b9;
    }
};
}  // namespace algos::cfdfinder