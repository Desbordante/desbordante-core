#pragma once

#include <cstddef>
#include <string>

#include <boost/functional/hash.hpp>

#include "core/algorithms/cfd/cfdfinder/model/pattern/entry.h"

namespace algos::cfdfinder {

class NegativeConstantEntry final : public Entry {
private:
    inline static std::string const kNegationSign = "¬";
    size_t constant_;

public:
    explicit NegativeConstantEntry(size_t constant) : constant_(constant) {}

    inline bool Matches(size_t value) const override final {
        return constant_ != value;
    }

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

    bool IsConstant() const override {
        return true;
    }

    std::string ToString(InvertedClusterMap const& cluster_map) const override {
        std::string value = cluster_map.at(constant_);

        return std::string(kNegationSign) +
               (!value.empty() ? value : std::string(kNullRepresentation));
    }
};
}  // namespace algos::cfdfinder
