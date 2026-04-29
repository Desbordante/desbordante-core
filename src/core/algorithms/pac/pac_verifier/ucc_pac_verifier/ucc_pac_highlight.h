#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <spdlog/fmt/bundled/format.h>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"
#include "core/model/types/type.h"

namespace algos::pac_verifier {
/// @brief Values that violate the UCC PAC with given epsilon
class UCCPACHighlight {
private:
    using TuplesIter = pac::model::Tuples::const_iterator;
    using Pairs = std::vector<TuplePair>;
    using PairsIter = Pairs::const_iterator;

    using RawPair = std::pair<pac::model::Tuple, pac::model::Tuple>;
    using StringPair = std::pair<std::string, std::string>;

    std::shared_ptr<pac::model::Tuples> tuples_;
    std::shared_ptr<pac::model::TupleType> tuple_type_;
    std::shared_ptr<Pairs> sorted_pairs_;

    // The range of values that form the highlight
    PairsIter begin_, end_;

    template <typename Res>
    std::vector<Res> TransformPairs(std::function<Res(pac::model::Tuple const&,
                                                      pac::model::Tuple const&)> const& op) const {
        std::vector<Res> result;
        result.reserve(std::distance(begin_, end_) * 2);
        auto apply_op = [this, &op](TuplePair const& p) {
            return op((*tuples_)[p.first_idx], (*tuples_)[p.second_idx]);
        };
        std::ranges::transform(begin_, end_, std::back_inserter(result), apply_op);
        std::ranges::transform(begin_, end_, std::back_inserter(result), apply_op,
                               &TuplePair::Inverse);
        return result;
    }

public:
    // Empty highlight
    UCCPACHighlight()
        : tuples_(std::make_shared<pac::model::Tuples>()),
          tuple_type_({}),
          sorted_pairs_(std::make_shared<Pairs>()),
          begin_(sorted_pairs_->begin()),
          end_(sorted_pairs_->end()) {}

    UCCPACHighlight(std::shared_ptr<pac::model::Tuples> tuples,
                    std::shared_ptr<pac::model::TupleType> tuple_type,
                    std::shared_ptr<Pairs> sorted_pairs, PairsIter&& begin, PairsIter&& end)
        : tuples_(std::move(tuples)),
          tuple_type_(std::move(tuple_type)),
          sorted_pairs_(std::move(sorted_pairs)),
          begin_(std::move(begin)),
          end_(std::move(end)) {}

    /// @brief Get pairs of row indices
    std::vector<std::pair<std::size_t, std::size_t>> RowIndices() const;

    /// @brief Get total number of pairs highlighted
    std::size_t NumPairs() const {
        // Each pair has a counterpart
        return 2 * std::distance(begin_, end_);
    }

    /// @brief Value @types associated with columns
    std::vector<model::Type const*> const& Types() const {
        return tuple_type_->GetTypes();
    }

    /// @brief Get pairs as pointers to @c std::byte that can be used with types
    std::vector<RawPair> RawData() const;

    /// @brief Get data as pairs of strings
    std::vector<StringPair> StringData() const;

    std::string ToString() const;
};
}  // namespace algos::pac_verifier
