#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "core/model/types/type.h"
#include "core/util/static_map.h"

namespace algos::fastadc {

enum class OperatorType { kEqual, kUnequal, kGreater, kLess, kGreaterEqual, kLessEqual };

constexpr std::array<OperatorType, 6> kAllOperatorTypes = {
        OperatorType::kEqual, OperatorType::kUnequal,      OperatorType::kGreater,
        OperatorType::kLess,  OperatorType::kGreaterEqual, OperatorType::kLessEqual};

/**
 * A small struct to enable range-based for on Operators.
 * It’s effectively a compile-time "view" into an array of OperatorType.
 */
class OperatorSpan {
    OperatorType const* data_;
    std::size_t size_;

public:
    constexpr OperatorSpan(OperatorType const* d, std::size_t n) : data_(d), size_(n) {}

    constexpr OperatorType const* begin() const {
        return data_;
    }

    constexpr OperatorType const* end() const {
        return data_ + size_;
    }
};

// TODO: remove code duplication cause we already have "dc/model/operator.h" that is used for
// DC verification.
class Operator {
    OperatorType op_;

    // Static arrays for all Implications and Transitives at compile time for each OperatorType:
    static constexpr OperatorType kEqImplications[] = {
            OperatorType::kEqual, OperatorType::kGreaterEqual, OperatorType::kLessEqual};
    static constexpr OperatorType kUneqImplications[] = {OperatorType::kUnequal};
    static constexpr OperatorType kGtImplications[] = {
            OperatorType::kGreater, OperatorType::kGreaterEqual, OperatorType::kUnequal};
    static constexpr OperatorType kLtImplications[] = {
            OperatorType::kLess, OperatorType::kLessEqual, OperatorType::kUnequal};
    static constexpr OperatorType kGeImplications[] = {OperatorType::kGreaterEqual};
    static constexpr OperatorType kLeImplications[] = {OperatorType::kLessEqual};
    static constexpr OperatorType kEqTransitives[] = {OperatorType::kEqual};
    static constexpr OperatorType kUneqTransitives[] = {OperatorType::kEqual};
    static constexpr OperatorType kGtTransitives[] = {
            OperatorType::kGreater, OperatorType::kGreaterEqual, OperatorType::kEqual};
    static constexpr OperatorType kLtTransitives[] = {OperatorType::kLess, OperatorType::kLessEqual,
                                                      OperatorType::kEqual};
    static constexpr OperatorType kGeTransitives[] = {
            OperatorType::kGreater, OperatorType::kGreaterEqual, OperatorType::kEqual};
    static constexpr OperatorType kLeTransitives[] = {OperatorType::kLess, OperatorType::kLessEqual,
                                                      OperatorType::kEqual};

    using OperatorMapType = util::StaticMap<OperatorType, OperatorType, 6>;
    using OperatorMapSpan = util::StaticMap<OperatorType, OperatorSpan, 6>;
    using OperatorMapString = util::StaticMap<OperatorType, std::string_view, 6>;

    static constexpr OperatorMapType kInverseMap{{{
            {OperatorType::kEqual, OperatorType::kUnequal},
            {OperatorType::kUnequal, OperatorType::kEqual},
            {OperatorType::kGreater, OperatorType::kLessEqual},
            {OperatorType::kLess, OperatorType::kGreaterEqual},
            {OperatorType::kGreaterEqual, OperatorType::kLess},
            {OperatorType::kLessEqual, OperatorType::kGreater},
    }}};

    static constexpr OperatorMapType kSymmetricMap{{{
            {OperatorType::kEqual, OperatorType::kEqual},
            {OperatorType::kUnequal, OperatorType::kUnequal},
            {OperatorType::kGreater, OperatorType::kLess},
            {OperatorType::kLess, OperatorType::kGreater},
            {OperatorType::kGreaterEqual, OperatorType::kLessEqual},
            {OperatorType::kLessEqual, OperatorType::kGreaterEqual},
    }}};

    static constexpr OperatorMapSpan kImplicationsMap{{{
            {OperatorType::kEqual, OperatorSpan(kEqImplications, 3)},
            {OperatorType::kUnequal, OperatorSpan(kUneqImplications, 1)},
            {OperatorType::kGreater, OperatorSpan(kGtImplications, 3)},
            {OperatorType::kLess, OperatorSpan(kLtImplications, 3)},
            {OperatorType::kGreaterEqual, OperatorSpan(kGeImplications, 1)},
            {OperatorType::kLessEqual, OperatorSpan(kLeImplications, 1)},
    }}};

    static constexpr OperatorMapSpan kTransitivesMap{{{
            {OperatorType::kEqual, OperatorSpan(kEqTransitives, 1)},
            {OperatorType::kUnequal, OperatorSpan(kUneqTransitives, 1)},
            {OperatorType::kGreater, OperatorSpan(kGtTransitives, 3)},
            {OperatorType::kLess, OperatorSpan(kLtTransitives, 3)},
            {OperatorType::kGreaterEqual, OperatorSpan(kGeTransitives, 3)},
            {OperatorType::kLessEqual, OperatorSpan(kLeTransitives, 3)},
    }}};

    static constexpr OperatorMapString kOperatorTypeToString{{{{OperatorType::kEqual, "=="},
                                                               {OperatorType::kUnequal, "!="},
                                                               {OperatorType::kGreater, ">"},
                                                               {OperatorType::kLess, "<"},
                                                               {OperatorType::kGreaterEqual, ">="},
                                                               {OperatorType::kLessEqual, "<="}}}};

public:
    Operator(OperatorType type) : op_(type) {}

    bool operator==(Operator const& rhs) const = default;
    bool operator!=(Operator const& rhs) const = default;

    bool Eval(std::byte const* v1, std::byte const* v2, model::Type const& type) const;

    // 'a op b' <=> !'a op.inverse b'
    Operator GetInverse() const {
        return Operator(kInverseMap.At(op_));
    }

    // 'a op b' <=> 'b op.symmetric a'
    Operator GetSymmetric() const {
        return Operator(kSymmetricMap.At(op_));
    }

    // If 'a op b', then 'a op.implications[i] b'
    OperatorSpan GetImplications() const {
        return kImplicationsMap.At(op_);
    }

    // If 'a op b' and 'b op.transitives[i] c', then 'a op c'
    OperatorSpan GetTransitives() const {
        return kTransitivesMap.At(op_);
    }

    std::string ToString() const {
        std::string_view sv = kOperatorTypeToString.At(op_);
        return std::string(sv);
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
