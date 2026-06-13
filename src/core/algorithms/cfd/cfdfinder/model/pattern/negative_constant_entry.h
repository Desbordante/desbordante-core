#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include <boost/functional/hash.hpp>

#include "core/algorithms/cfd/cfdfinder/model/pattern/entry.h"

namespace algos::cfdfinder {

class NegativeConstantEntry final : public Entry {
private:
    inline static constexpr std::string_view kNegationSign = "¬";
    size_t constant_;

public:
    explicit NegativeConstantEntry(size_t constant) : constant_(constant) {}

    bool operator==(Entry const& other) const override final {
        auto const* other_constant = dynamic_cast<NegativeConstantEntry const*>(&other);
        return other_constant != nullptr && constant_ == other_constant->constant_;
    }

    size_t Hash() const override {
        return boost::hash_value(constant_) * 31;
    }

    size_t GetConstant() const {
        return constant_;
    }

    bool IsConstantType() const override {
        return true;
    }

    std::string ToString(InvertedClusterMap const& cluster_map) const override {
        std::string value = cluster_map.at(constant_);

        return std::string(kNegationSign) +
               (!value.empty() ? value : std::string(kNullRepresentation));
    }

    int GetOrderRank() const override {
        return 2;
    }

    bool operator<(Entry const& other) const override {
        if (GetOrderRank() != other.GetOrderRank()) return GetOrderRank() < other.GetOrderRank();

        auto const& other_negative = static_cast<NegativeConstantEntry const&>(other);
        return constant_ < other_negative.constant_;
    }
};
}  // namespace algos::cfdfinder
