#pragma once

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "closure.h"
#include "dc/ntree_search.h"
#include "denial_constraint.h"
#include "predicate_set.h"

namespace algos::fastadc {

// Java way of comparing bitsets
int CompareBitsets(boost::dynamic_bitset<> const& lhs, boost::dynamic_bitset<> const& rhs) {
    size_t max_size = std::max(lhs.size(), rhs.size());

    for (size_t i = 0; i < max_size; ++i) {
        bool lhs_bit = i < lhs.size() ? lhs[i] : false;
        bool rhs_bit = i < rhs.size() ? rhs[i] : false;

        if (lhs_bit != rhs_bit) {
            return lhs_bit ? 1 : -1;  // lhs_bit is true and rhs_bit is false => lhs > rhs
        }
    }
    return 0;  // bitsets are equal
}

class DenialConstraintSet {
private:
    std::unordered_set<DenialConstraint> constraints_;
    std::vector<DenialConstraint> result_;

    struct MinimalDCCandidate {
        DenialConstraint const* dc = nullptr;
        boost::dynamic_bitset<> bitset{64};

        MinimalDCCandidate() = default;

        MinimalDCCandidate(DenialConstraint const& dc)
            : dc(&dc), bitset(dc.GetPredicateSet().GetBitset()) {}

        int Compare(MinimalDCCandidate const& other) const {
            if (dc->GetPredicateCount() < other.dc->GetPredicateCount()) return -1;
            if (dc->GetPredicateCount() > other.dc->GetPredicateCount()) return 1;

            int bitset_cmp = CompareBitsets(bitset, other.bitset);
            return bitset_cmp;
        }

        bool ShouldReplace(MinimalDCCandidate const& prior) const {
            return Compare(prior) <= 0;
        }
    };

    struct Comparator {
        bool operator()(std::pair<PredicateSet, MinimalDCCandidate> const& a,
                        std::pair<PredicateSet, MinimalDCCandidate> const& b) const {
            if (a.first.Size() != b.first.Size()) return a.first.Size() < b.first.Size();
            int cmp = a.second.Compare(b.second);
            if (cmp != 0) return cmp < 0;

            cmp = CompareBitsets(a.first.GetBitset(), b.first.GetBitset());
            return cmp < 0;
        }
    };

public:
    DenialConstraintSet() = default;

    bool Contains(DenialConstraint const& dc) const {
        return constraints_.contains(dc);
    }

    void Add(DenialConstraint const& dc) {
        constraints_.insert(dc);
    }

    size_t TotalDCSize() const {
        return constraints_.size();
    }

    size_t MinDCSize() const {
        return result_.size();
    }

    std::string ToString() const {
        std::string out;
        for (auto const& dc : result_) out += dc.ToString() + "\n";
        return out;
    }

    std::vector<DenialConstraint>&& GetResult() {
        return std::move(result_);
    }

    // Minimize the set of denial constraints
    void Minimize() {
        std::unordered_map<PredicateSet, MinimalDCCandidate> constraints_closure_map;

        // Construct closures for each denial constraint
        for (auto const& dc : constraints_) {
            PredicateSet predicate_set = dc.GetPredicateSet();
            Closure closure(predicate_set);
            if (closure.Construct()) {
                MinimalDCCandidate candidate(dc);
                PredicateSet closure_set = closure.GetClosure();

                auto it = constraints_closure_map.find(closure_set);
                if (it == constraints_closure_map.end() || candidate.ShouldReplace(it->second)) {
                    constraints_closure_map[closure_set] = candidate;
                }
            }
        }

        // Convert map to vector for sorting
        std::vector<std::pair<PredicateSet, MinimalDCCandidate>> constraints_vector(
                constraints_closure_map.begin(), constraints_closure_map.end());

        // Sort constraints by closure size and predicate count
        std::sort(constraints_vector.begin(), constraints_vector.end(), Comparator());

        // Process minimized constraints
        NTreeSearch tree;
        for (auto const& [closure_set, candidate] : constraints_vector) {
            if (tree.ContainsSubset(closure_set.GetBitset())) {
                continue;
            }

            DenialConstraint inv = candidate.dc->GetInvT1T2DC();
            if (inv.GetPredicateSet().Size() > 0) {
                Closure inv_closure(inv.GetPredicateSet());
                if (!inv_closure.Construct()) {
                    continue;
                }
                if (tree.ContainsSubset(inv_closure.GetClosure().GetBitset())) {
                    continue;
                }
            }

            // TODO: std::move?
            result_.push_back(std::move(*candidate.dc));
            tree.Add(candidate.bitset);
            if (inv.GetPredicateSet().Size() > 0) {
                tree.Add(inv.GetPredicateSet().GetBitset());
            }
        }
    }
};

}  // namespace algos::fastadc
