#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "type.h"

namespace algos::fastadc {

enum class OperatorType { kEqual, kUnequal, kGreater, kLess, kGreaterEqual, kLessEqual };

constexpr std::array<OperatorType, 6> kAllOperatorTypes = {
        OperatorType::kEqual, OperatorType::kUnequal,      OperatorType::kGreater,
        OperatorType::kLess,  OperatorType::kGreaterEqual, OperatorType::kLessEqual};

// TODO: remove code duplication cause we already have "dc/model/operator.h" that is used for
// DC verification.
class Operator {
private:
    OperatorType op_;

    template <typename V>
    using OperatorMap = std::unordered_map<OperatorType, V>;

    static OperatorMap<OperatorType> const kInverseMap;
    static OperatorMap<OperatorType> const kSymmetricMap;
    static OperatorMap<std::vector<OperatorType>> const kImplicationsMap;
    static OperatorMap<std::vector<OperatorType>> const kTransitivesMap;
    static OperatorMap<std::string> const kShortStringMap;

    static OperatorMap<OperatorType> InitializeInverseMap();
    static OperatorMap<OperatorType> InitializeSymmetricMap();
    static OperatorMap<std::vector<OperatorType>> InitializeImplicationsMap();
    static OperatorMap<std::vector<OperatorType>> InitializeTransitivesMap();
    static OperatorMap<std::string> InitializeShortStringMap();

public:
    Operator(OperatorType type) : op_(type) {}

    bool operator==(Operator const& rhs) const = default;
    bool operator!=(Operator const& rhs) const = default;

    bool Eval(std::byte const* v1, std::byte const* v2, model::Type const& type) const;

    // 'a op b' <=> !'a op.inverse b'
    Operator GetInverse() const {
        return Operator(kInverseMap.at(op_));
    }

    // 'a op b' <=> 'b op.symmetric a'
    Operator GetSymmetric() const {
        return Operator(kSymmetricMap.at(op_));
    }

    // If 'a op b', then 'a op.implications[i] b'
    std::vector<Operator> GetImplications() const {
        std::vector<Operator> implications;
        for (auto type : kImplicationsMap.at(op_)) {
            implications.emplace_back(type);
        }
        return implications;
    }

    // If 'a op b' and 'b op.transitives[i] c', then 'a op c'
    std::vector<Operator> GetTransitives() const {
        std::vector<Operator> transitives;
        for (auto type : kTransitivesMap.at(op_)) {
            transitives.emplace_back(type);
        }
        return transitives;
    }

    std::string ToString() const {
        return kShortStringMap.at(op_);
    }

    OperatorType GetType() const {
        return op_;
    }
};

// NOLINTBEGIN(readability-identifier-naming)
size_t hash_value(Operator const& k) noexcept;
// NOLINTEND(readability-identifier-naming)

}  // namespace algos::fastadc

template <>
struct std::hash<algos::fastadc::Operator> {
    size_t operator()(algos::fastadc::Operator const& k) const noexcept {
        return static_cast<size_t>(k.GetType());
    }
};
