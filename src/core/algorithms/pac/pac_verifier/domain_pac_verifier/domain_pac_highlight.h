#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "algorithms/pac/model/tuple.h"
#include "pac/model/comparable_tuple_type.h"
#include "pac/pac_verifier/pac_highlight.h"
#include "type.h"

namespace algos::pac_verifier {
/// @brief Values that violate Domain PAC with given epsilon
// Looks like there's no Domain PAC-specific things, maybe this class may become PACHighlight?
class DomainPACHighlight : public PACHighlight {
private:
    using Tuples = std::vector<pac::model::Tuple>;
    using TuplesIter = Tuples::iterator;

    std::shared_ptr<pac::model::ComparableTupleType> tuple_type_;
    std::shared_ptr<Tuples> original_value_tuples_;
    std::vector<TuplesIter> highlighted_tuples_;

public:
    DomainPACHighlight(std::shared_ptr<pac::model::ComparableTupleType> tuple_type,
                       std::shared_ptr<Tuples> original_value_tuples,
                       std::vector<TuplesIter>&& highlighted_tuples)
        : tuple_type_(std::move(tuple_type)),
          original_value_tuples_(std::move(original_value_tuples)),
          highlighted_tuples_(std::move(highlighted_tuples)) {}

    /// @brief Get row numbers of highlighted values
    virtual std::vector<std::size_t> GetRowNums() const override {
        std::vector<std::size_t> indices;
        std::ranges::transform(highlighted_tuples_, std::back_inserter(indices),
                               [beg = original_value_tuples_->begin()](auto const it) {
                                   return std::distance(beg, it);
                               });
        return indices;
    }

    /// @brief Get @c Types of columns associated with this @c Highlight
    virtual std::vector<model::Type const*> const& GetTypes() const override {
        return tuple_type_->GetTypes();
    }

    /// @brief Get highlighted values as pointers to @c std::byte, that can be used with types (see
    /// @c GetTypes())
    virtual Tuples GetByteData() const override {
        Tuples tuples;
        std::ranges::transform(highlighted_tuples_, std::back_inserter(tuples),
                               [](auto const it) { return *it; });
        return tuples;
    }

    /// @brief Get highlighted values as strings
    virtual std::vector<std::string> GetStringData() const override {
        std::vector<std::string> strings;
        std::ranges::transform(highlighted_tuples_, std::back_inserter(strings),
                               [this](auto const it) { return tuple_type_->ValueToString(*it); });
        return strings;
    }
};
}  // namespace algos::pac_verifier
