#include "dc/FastADC/util/dc_candidate_trie.h"

#include "dc/FastADC/model/predicate.h"
#include "dc/FastADC/util/dc_candidate.h"

namespace algos::fastadc {

DCCandidateTrie::DCCandidateTrie(size_t max_subtrees) : max_subtrees_(max_subtrees) {
    subtrees_.resize(max_subtrees_);
}

bool DCCandidateTrie::Add(DCCandidate const& add_dc) {
    boost::dynamic_bitset<> const& bitset = add_dc.bitset;
    DCCandidateTrie* tree_node = this;

    for (size_t i = bitset.find_first(); i != boost::dynamic_bitset<>::npos;
         i = bitset.find_next(i)) {
        auto& subtree = tree_node->subtrees_[i];
        if (!subtree) {
            subtree = std::make_unique<DCCandidateTrie>(max_subtrees_);
        }
        tree_node = subtree.get();
    }

    tree_node->dc_ = add_dc;
    return true;
}

std::vector<DCCandidate> DCCandidateTrie::GetAndRemoveGeneralizations(
        PredicateBitset const& superset) {
    std::vector<DCCandidate> removed;

    boost::dynamic_bitset<> dynamic_superset(kPredicateBits);
    for (size_t i = 0; i < kPredicateBits; ++i) {
        if (superset.test(i)) {
            dynamic_superset.set(i);
        }
    }

    GetAndRemoveGeneralizationsAux(dynamic_superset, removed);
    return removed;
}

void DCCandidateTrie::GetAndRemoveGeneralizationsAux(boost::dynamic_bitset<> const& superset,
                                                     std::vector<DCCandidate>& removed) {
    if (dc_) {
        removed.push_back(*dc_);
        dc_.reset();
    }

    for (size_t i = superset.find_first(); i != boost::dynamic_bitset<>::npos;
         i = superset.find_next(i)) {
        auto& subtree = subtrees_[i];
        if (subtree) {
            subtree->GetAndRemoveGeneralizationsAux(superset, removed);
            if (subtree->IsEmpty()) {
                subtree.reset();
            }
        }
    }
}

bool DCCandidateTrie::IsEmpty() const {
    return !dc_ && NoSubtree();
}

bool DCCandidateTrie::NoSubtree() const {
    return std::ranges::none_of(subtrees_, [](auto const& ptr) { return !!ptr; });
}

bool DCCandidateTrie::ContainsSubset(DCCandidate const& add) {
    return GetSubset(add) != nullptr;
}

DCCandidate* DCCandidateTrie::GetSubset(DCCandidate const& add) {
    return GetSubsetAux(add);
}

DCCandidate* DCCandidateTrie::GetSubsetAux(DCCandidate const& add) {
    if (dc_) {
        return &(dc_.value());
    }

    boost::dynamic_bitset<> const& bitset = add.bitset;

    for (size_t i = bitset.find_first(); i != boost::dynamic_bitset<>::npos;
         i = bitset.find_next(i)) {
        auto& subtree = subtrees_[i];
        if (subtree) {
            DCCandidate* res = subtree->GetSubsetAux(add);
            if (res != nullptr) {
                return res;
            }
        }
    }
    return nullptr;
}

void DCCandidateTrie::ForEach(std::function<void(DCCandidate const&)> const& consumer) {
    if (dc_) {
        consumer(*dc_);
    }
    for (auto const& subtree : subtrees_) {
        if (subtree) {
            subtree->ForEach(consumer);
        }
    }
}

}  // namespace algos::fastadc
