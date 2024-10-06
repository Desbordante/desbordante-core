#pragma once

#include "../model/evidence_set.h"

namespace algos::fastadc {

// re-order predicates by evidence coverage to accelerate trie
class PredicateOrganizer {
public:
    PredicateOrganizer(size_t n_predicates, EvidenceSet&& evidence_set,
                       std::vector<PredicateBitset>&& mutex_map)
        : n_predicates_(n_predicates),
          evidence_set_(std::move(evidence_set)),
          mutex_map_(std::move(mutex_map)) {
        indexes_ = CreateIndexArray(GetPredicateCoverage());
    }

    std::vector<Evidence> TransformEvidenceSet() const {
        std::vector<Evidence> transformed_evidences;
        transformed_evidences.reserve(evidence_set_.Size());

        for (auto const& evidence : evidence_set_) {
            transformed_evidences.emplace_back(Transform(evidence.evidence), evidence.count);
        }

        return transformed_evidences;
    }

    std::vector<PredicateBitset> TransformMutexMap() const {
        std::vector<PredicateBitset> trans_mutex_map(mutex_map_.size());

        for (size_t i = 0; i < mutex_map_.size(); i++) {
            trans_mutex_map[Transform(i)] = Transform(mutex_map_[i]);
        }

        return trans_mutex_map;
    }

    int Transform(size_t e) const {
        auto it = std::find(indexes_.begin(), indexes_.end(), e);
        return (it != indexes_.end()) ? std::distance(indexes_.begin(), it) : -1;
    }

    PredicateBitset Transform(PredicateBitset const& bitset) const {
        PredicateBitset transformed_bitset;

        for (size_t i : indexes_) {
            if (bitset.test(indexes_[i])) {
                transformed_bitset.set(i);
            }
        }

        return transformed_bitset;
    }

    boost::dynamic_bitset<> Retransform(boost::dynamic_bitset<> const& bitset) const {
        boost::dynamic_bitset<> valid{kPredicateBits};

        for (size_t i = bitset.find_first(); i != boost::dynamic_bitset<>::npos;
             i = bitset.find_next(i)) {
            valid.set(indexes_[i]);  // indexes_[i] is <= than number of predicates
        }

        return valid;
    }

private:
    size_t n_predicates_;
    EvidenceSet evidence_set_;
    std::vector<PredicateBitset> mutex_map_;
    // new index -> original index
    std::vector<int> indexes_;

    std::vector<int> GetPredicateCoverage() const {
        std::vector<int> counts(n_predicates_, 0);

        for (auto const& evidence : evidence_set_) {
            PredicateBitset bitset = evidence.evidence;
            for (size_t i = bitset._Find_first(); i != bitset.size(); i = bitset._Find_next(i)) {
                counts[i]++;
            }
        }

        return counts;
    }

    std::vector<int> CreateIndexArray(std::vector<int> const& coverages) const {
        std::vector<int> indexes(coverages.size());
        std::iota(indexes.begin(), indexes.end(), 0);

        std::stable_sort(indexes.begin(), indexes.end(),
                         [&](int i, int j) { return coverages[i] < coverages[j]; });

        return indexes;
    }
};

}  // namespace algos::fastadc
