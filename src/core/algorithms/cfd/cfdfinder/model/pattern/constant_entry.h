#pragma once

#include <cstddef>
#include <string>
#include <tuple>

#include <boost/functional/hash.hpp>

#include "core/algorithms/cfd/cfdfinder/model/pattern/entry.h"

namespace algos::cfdfinder {

class ConstantEntry final : public Entry {
private:
    size_t constant_;

public:
    explicit ConstantEntry(size_t constant) : constant_(constant) {}

    bool operator==(Entry const& other) const override final {
        auto const* other_constant = dynamic_cast<ConstantEntry const*>(&other);
        return other_constant != nullptr && constant_ == other_constant->constant_;
    }

    bool operator!=(Entry const& other) const {
        return !(*this == other);
    }

    size_t Hash() const override {
        return boost::hash_value(constant_);
    }

    size_t GetConstant() const {
        return constant_;
    }

    bool IsConstantType() const override {
        return true;
    }

    inline int GetOrderRank() const override {
        return 1;
    }

    bool operator<(Entry const& other) const override {
        if (GetOrderRank() != other.GetOrderRank()) return GetOrderRank() < other.GetOrderRank();

        auto const& other_constant = static_cast<ConstantEntry const&>(other);
        return constant_ < other_constant.constant_;
    }

    std::string ToString(InvertedClusterMap const& cluster_map) const override {
        std::string value = cluster_map.at(constant_);

        return !value.empty() ? value : std::string(kNullRepresentation);
    }
};
}  // namespace algos::cfdfinder
