#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"
#include "core/model/types/type.h"

namespace algos::pac_verifier {
class FDPACHighlight {
private:
    using Pairs = std::vector<TuplePair>;
    using Tuples = pac::model::Tuples;
    using Types = std::vector<model::Type const*>;

    using PairsIt = Pairs::const_iterator;

    using LhsRhsData = std::pair<pac::model::Tuple, pac::model::Tuple>;
    using RawPair = std::pair<LhsRhsData, LhsRhsData>;
    using LhsRhsString = std::pair<std::string, std::string>;
    using StringPair = std::pair<LhsRhsString, LhsRhsString>;

    std::shared_ptr<Tuples> lhs_tuples_;
    std::shared_ptr<Tuples> rhs_tuples_;
    std::shared_ptr<Types> lhs_types_;
    std::shared_ptr<Types> rhs_types_;
    // Pairs are kept here to extend their lifetime
    std::shared_ptr<Pairs> pairs_;
    // Range of pairs that form the highlight
    PairsIt begin_, end_;

    template <typename Res>
    std::vector<Res> TransformPairs(std::function<Res(TuplePair const&)> const& op) const {
        std::vector<Res> result;
        result.reserve(std::distance(begin_, end_) * 2);
        std::ranges::transform(begin_, end_, std::back_inserter(result), op);
        std::ranges::transform(begin_, end_, std::back_inserter(result), op, &TuplePair::Inverse);
        return result;
    }

public:
    // Empty highlight
    FDPACHighlight()
        : lhs_tuples_(std::make_shared<Tuples>()),
          rhs_tuples_(std::make_shared<Tuples>()),
          lhs_types_(std::make_shared<Types>()),
          rhs_types_(std::make_shared<Types>()),
          pairs_(std::make_shared<Pairs>()),
          begin_(pairs_->begin()),
          end_(pairs_->end()) {}

    FDPACHighlight(std::shared_ptr<Tuples> lhs_tuples, std::shared_ptr<Tuples> rhs_tuples,
                   std::shared_ptr<Types> lhs_types, std::shared_ptr<Types> rhs_types,
                   std::shared_ptr<Pairs> pairs, PairsIt begin, PairsIt end)
        : lhs_tuples_(std::move(lhs_tuples)),
          rhs_tuples_(std::move(rhs_tuples)),
          lhs_types_(std::move(lhs_types)),
          rhs_types_(std::move(rhs_types)),
          pairs_(std::move(pairs)),
          begin_(begin),
          end_(end) {}

    /// @brief Get pairs of row indices
    std::vector<std::pair<std::size_t, std::size_t>> RowIndices() const;

    /// @brief Get total number of pairs highlighted
    std::size_t NumPairs() const {
        // Each pair has a counterpart
        return 2 * std::distance(begin_, end_);
    }

    /// @brief Value @c types associated with left-hand side columns
    std::vector<model::Type const*> const& LhsTypes() const {
        return *lhs_types_;
    }

    /// @brief Value @c types associated with right-hand side columns
    std::vector<model::Type const*> const& RhsTypes() const {
        return *rhs_types_;
    }

    /// @brief Get pairs as pointers to @c std::byte that can be used with types (see @c LhsTypes,
    /// @c RhsTypes)
    std::vector<RawPair> RawData() const;

    /// @brief Get data as pairs of strings
    std::vector<StringPair> StringData() const;

    std::string ToString() const;
};
}  // namespace algos::pac_verifier
