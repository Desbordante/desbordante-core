#pragma once

#include <cstddef>
#include <string>

#include <boost/functional/hash.hpp>

#include "algorithms/cfd/cfdfinder/model/pattern/entry.h"

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
        auto const* other_constant = dynamic_cast<ConstantEntry const*>(&other);
        return other_constant != nullptr && constant_ == other_constant->constant_;
    }

    size_t Hash() const override {
        return boost::hash_value(constant_);
    }

    size_t GetConstant() const {
        return constant_;
    }

    bool IsConstant() const override {
        return true;
    }

    std::string ToString(InvertedClusterMap const& cluster_map) const override {
        std::string value = cluster_map.at(constant_);

        return !value.empty() ? value : kNullRepresentation;
    }
};
}  // namespace algos::cfdfinder
