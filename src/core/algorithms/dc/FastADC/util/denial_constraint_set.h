#pragma once

#include "core/algorithms/dc/FastADC/model/denial_constraint.h"
#include "core/algorithms/dc/FastADC/util/closure.h"
#include "core/algorithms/dc/FastADC/util/ntree_search.h"

namespace algos::fastadc {

class DenialConstraintSet {
private:
    std::unordered_set<DenialConstraint> constraints_;
    std::vector<DenialConstraint> result_;
    PredicateProvider* predicate_provider_;

    using CompareResult = model::CompareResult;

    // Java way of comparing bitsets
    static CompareResult CompareBitsets(boost::dynamic_bitset<> const& lhs,
                                        boost::dynamic_bitset<> const& rhs) {
        size_t max_size = std::max(lhs.size(), rhs.size());

        for (size_t i = 0; i < max_size; ++i) {
            bool lhs_bit = (i < lhs.size()) ? lhs[i] : false;
            bool rhs_bit = (i < rhs.size()) ? rhs[i] : false;

            if (lhs_bit != rhs_bit) {
                return lhs_bit ? CompareResult::kGreater : CompareResult::kLess;
            }
        }

        return CompareResult::kEqual;
    }

    struct MinimalDCCandidate {
        DenialConstraint const* dc = nullptr;
        boost::dynamic_bitset<> bitset{kPredicateBits};

        MinimalDCCandidate() = default;

        MinimalDCCandidate(DenialConstraint const& dc)
            : dc(&dc), bitset(dc.GetPredicateSet().GetBitset()) {}

        CompareResult Compare(MinimalDCCandidate const& other) const {
            if (dc->GetPredicateCount() < other.dc->GetPredicateCount()) {
                return CompareResult::kLess;
            }
            if (dc->GetPredicateCount() > other.dc->GetPredicateCount()) {
                return CompareResult::kGreater;
            }

            return CompareBitsets(bitset, other.bitset);
        }

        bool ShouldReplace(MinimalDCCandidate const& prior) const {
            CompareResult cmp = Compare(prior);
            return (cmp == CompareResult::kLess) || (cmp == CompareResult::kEqual);
        }
    };

    struct Comparator {
        bool operator()(std::pair<PredicateSet, MinimalDCCandidate> const& a,
                        std::pair<PredicateSet, MinimalDCCandidate> const& b) const {
            if (a.first.Size() != b.first.Size()) {
                return a.first.Size() < b.first.Size();
            }

            model::CompareResult cmp = a.second.Compare(b.second);
            if (cmp == model::CompareResult::kLess) {
                return true;
            }
            if (cmp == model::CompareResult::kGreater) {
                return false;
            }

            CompareResult bitset_cmp = CompareBitsets(a.first.GetBitset(), b.first.GetBitset());
            return bitset_cmp == CompareResult::kLess;
        }
    };

public:
    DenialConstraintSet(PredicateProvider* predicate_provider)
        : predicate_provider_(predicate_provider) {
        assert(predicate_provider);
    }

    DenialConstraintSet() : predicate_provider_(nullptr) {}

    DenialConstraintSet(DenialConstraintSet const& other) = delete;
    DenialConstraintSet& operator=(DenialConstraintSet const& other) = delete;
    DenialConstraintSet(DenialConstraintSet&& other) noexcept = default;
    DenialConstraintSet& operator=(DenialConstraintSet&& other) noexcept = default;

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
        std::stringstream ss;
        for (DenialConstraint const& dc : result_) {
            ss << dc.ToString() << "\n";
        }
        return ss.str();
    }

    std::vector<DenialConstraint>&& ObtainResult() {
        return std::move(result_);
    }

    std::vector<DenialConstraint> const& GetResult() const {
        return result_;
    }

    void Clear() {
        constraints_.clear();
        result_.clear();
        predicate_provider_ = nullptr;
    }

    // Minimize the set of denial constraints
    void Minimize() {
        std::unordered_map<PredicateSet, MinimalDCCandidate> constraints_closure_map;

        // Construct closures for each denial constraint
        for (auto const& dc : constraints_) {
            PredicateSet predicate_set = dc.GetPredicateSet();
            Closure closure(predicate_set, predicate_provider_);
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

            DenialConstraint inv = candidate.dc->GetInvT1T2DC(predicate_provider_);
            if (inv.GetPredicateSet().Size() > 0) {
                Closure inv_closure(inv.GetPredicateSet(), predicate_provider_);
                if (!inv_closure.Construct()) {
                    continue;
                }
                if (tree.ContainsSubset(inv_closure.GetClosure().GetBitset())) {
                    continue;
                }
            }

            // TODO: std::move?
            result_.push_back(std::move(*candidate.dc));
            tree.Insert(candidate.bitset);
            if (inv.GetPredicateSet().Size() > 0) {
                tree.Insert(inv.GetPredicateSet().GetBitset());
            }
        }
    }
};

}  // namespace algos::fastadc
