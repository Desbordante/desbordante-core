#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <stddef.h>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "dc/FastADC/model/predicate.h"
#include "dc/FastADC/util/dc_candidate.h"

namespace algos::fastadc {
class DCCandidateTrie {
public:
    explicit DCCandidateTrie(size_t max_subtrees);

    bool Add(DCCandidate const& addDC);

    std::vector<DCCandidate> GetAndRemoveGeneralizations(PredicateBitset const& superset);

    bool IsEmpty() const;

    bool ContainsSubset(DCCandidate const& add);

    DCCandidate* GetSubset(DCCandidate const& add);

    // TODO: F&& consumer to avoid creating std::function object if lambda is passed
    void ForEach(std::function<void(DCCandidate const&)> const& consumer);

private:
    std::vector<std::unique_ptr<DCCandidateTrie>> subtrees_;
    std::optional<DCCandidate> dc_;
    size_t max_subtrees_;

    void GetAndRemoveGeneralizationsAux(boost::dynamic_bitset<> const& superset,
                                        std::vector<DCCandidate>& removed);

    DCCandidate* GetSubsetAux(DCCandidate const& add);

    int NextSetBit(boost::dynamic_bitset<> const& bitset, int pos) const;

    bool NoSubtree() const;
};
}  // namespace algos::fastadc
