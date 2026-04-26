#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"
#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"

namespace algos::pac_verifier {
/// @brief Values that violate the UCC PAC with given epsilon
class UCCPACHighlight {
private:
    using TuplesIter = pac::model::Tuples::const_iterator;
    using Pairs = std::vector<TuplePair>;
    using PairsIter = Pairs::const_iterator;

    std::shared_ptr<pac::model::Tuples> tuples_;
    std::shared_ptr<pac::model::TupleType> tuple_type_;
    std::shared_ptr<Pairs> sorted_pairs_;

    // The range of values that form the highlight
    PairsIter begin_, end_;

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
};
}  // namespace algos::pac_verifier
