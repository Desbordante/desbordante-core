#pragma once

#include <algorithm>
#include <cmath>
#include <stack>
#include <vector>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <easylogging++.h>

#include "dc/FastADC/model/evidence_set.h"
#include "dc/FastADC/util/dc_candidate_trie.h"
#include "dc/FastADC/util/denial_constraint_set.h"
#include "dc/FastADC/util/predicate_builder.h"
#include "dc/FastADC/util/predicate_organizer.h"

namespace algos::fastadc {

inline boost::dynamic_bitset<>& operator&=(boost::dynamic_bitset<>& lhs,
                                           PredicateBitset const& rhs) {
    for (size_t i = 0; i < std::min(rhs.size(), lhs.size()); ++i) {
        lhs[i] &= rhs[i];
    }
    return lhs;
}

inline boost::dynamic_bitset<> operator&(boost::dynamic_bitset<> const& lhs,
                                         PredicateBitset const& rhs) {
    boost::dynamic_bitset<> result(lhs);
    result &= rhs;
    return result;
}

class ApproxEvidenceInverter {
public:
    ApproxEvidenceInverter(PredicateBuilder& pbuilder, double threshold, EvidenceSet&& evidence_set)
        : n_predicates_(pbuilder.PredicateCount()),
          evi_count_(evidence_set.GetTotalCount()),
          target_(static_cast<int64_t>(std::ceil((1 - threshold) * evi_count_))),
          organizer_(n_predicates_, std::move(evidence_set), std::move(pbuilder.TakeMutexMap())),
          approx_covers_(n_predicates_),
          predicate_provider_(pbuilder.predicate_provider),
          predicate_index_provider_(pbuilder.predicate_index_provider) {
        LOG(DEBUG) << " [AEI] Violate at most " << evi_count_ - target_ << " tuple pairs";
        evidences_ = organizer_.TransformEvidenceSet();
        mutex_map_ = organizer_.TransformMutexMap();
    }

    ApproxEvidenceInverter(ApproxEvidenceInverter const& other) = delete;
    ApproxEvidenceInverter& operator=(ApproxEvidenceInverter const& other) = delete;
    ApproxEvidenceInverter(ApproxEvidenceInverter&& other) noexcept = default;
    ApproxEvidenceInverter& operator=(ApproxEvidenceInverter&& other) noexcept = default;

    DenialConstraintSet BuildDenialConstraints() {
        if (target_ == 0) return {predicate_provider_};

        auto cmp = [](Evidence const& o1, Evidence const& o2) { return o1.count > o2.count; };
        std::ranges::sort(evidences_, cmp);

        InverseEvidenceSet();

        std::vector<boost::dynamic_bitset<>> raw_dcs;
        approx_covers_.ForEach([this, &raw_dcs](DCCandidate const& transDC) {
            raw_dcs.push_back(organizer_.Retransform(transDC.bitset));
        });

        LOG(DEBUG) << "  [AEI] Min cover size: " << raw_dcs.size();

        DenialConstraintSet constraints(predicate_provider_);
        for (auto const& raw_dc : raw_dcs)
            constraints.Add(DenialConstraint(raw_dc, predicate_index_provider_));

        LOG(DEBUG) << "  [AEI] Total DC size: " << constraints.TotalDCSize();

        constraints.Minimize();

        LOG(DEBUG) << "  [AEI] Min DC size : " << constraints.MinDCSize();

        return constraints;
    }

private:
    size_t n_predicates_;
    size_t evi_count_;
    int64_t target_;
    std::vector<Evidence> evidences_;
    std::vector<PredicateBitset> mutex_map_;
    PredicateOrganizer organizer_;
    DCCandidateTrie approx_covers_;
    PredicateProvider* predicate_provider_;
    PredicateIndexProvider* predicate_index_provider_;

    struct SearchNode {
        size_t e;
        boost::dynamic_bitset<> addable_predicates;
        DCCandidateTrie dc_candidates;
        std::vector<DCCandidate> invalid_dcs;
        int64_t target;

        SearchNode(size_t e, boost::dynamic_bitset<> const& addable_predicates,
                   DCCandidateTrie&& dc_candidates, std::vector<DCCandidate> const& invalid_dcs,
                   int64_t target)
            : e(e),
              addable_predicates(addable_predicates),
              dc_candidates(std::move(dc_candidates)),
              invalid_dcs(invalid_dcs),
              target(target) {}
    };

    void InverseEvidenceSet() {
        LOG(DEBUG) << "  [AEI] Inverting evidences...";

        approx_covers_ = DCCandidateTrie(n_predicates_);
        boost::dynamic_bitset<> full_mask(n_predicates_);
        full_mask.set();
        std::stack<SearchNode> nodes;  // Manual stack, where evidences_[node.e] needs to be hit

        DCCandidateTrie dc_candidates(n_predicates_);
        dc_candidates.Add(DCCandidate{.cand = full_mask});

        Walk(0, full_mask, std::move(dc_candidates), target_, nodes);

        while (!nodes.empty()) {
            SearchNode nd = std::move(nodes.top());
            nodes.pop();
            if (nd.e >= evidences_.size() || nd.addable_predicates.none()) continue;
            Hit(nd);  // Hit evidences_[e]
            if (nd.target > 0)
                Walk(nd.e + 1, nd.addable_predicates, std::move(nd.dc_candidates), nd.target,
                     nodes);
        }
    }

    void Walk(size_t e, boost::dynamic_bitset<>& addable_predicates,
              DCCandidateTrie&& dc_candidates, int64_t target, std::stack<SearchNode>& nodes) {
        while (e < evidences_.size() && !dc_candidates.IsEmpty()) {
            PredicateBitset evi = evidences_[e].evidence;
            auto unhit_evi_dcs = dc_candidates.GetAndRemoveGeneralizations(evi);

            // Hit evidences_[e] later
            nodes.push(SearchNode(e, addable_predicates, std::move(dc_candidates), unhit_evi_dcs,
                                  target));

            // Unhit evidences_[e]
            if (unhit_evi_dcs.empty()) return;

            addable_predicates &= evi;

            if (addable_predicates.none()) return;

            int64_t max_can_hit = 0;
            for (size_t i = e + 1; i < evidences_.size(); ++i)
                if (!(IsSubset(addable_predicates, evidences_[i].evidence)))
                    max_can_hit += evidences_[i].count;
            if (max_can_hit < target) return;

            DCCandidateTrie new_candidates(n_predicates_);
            for (auto const& dc : unhit_evi_dcs) {
                boost::dynamic_bitset<> unhit_cand = dc.cand & evi;
                if (unhit_cand.any())
                    new_candidates.Add(DCCandidate{dc.bitset, unhit_cand});
                else if (!approx_covers_.ContainsSubset(dc) &&
                         IsApproxCover(dc.bitset, e + 1, target))
                    approx_covers_.Add(dc);
            }
            if (new_candidates.IsEmpty()) return;

            e++;
            dc_candidates = std::move(new_candidates);
        }
    }

    void Hit(SearchNode& nd) {
        if (nd.e >= evidences_.size() || IsSubset(nd.addable_predicates, evidences_[nd.e].evidence))
            return;

        nd.target -= evidences_[nd.e].count;

        PredicateBitset evi = evidences_[nd.e].evidence;
        auto& dc_candidates = nd.dc_candidates;

        if (nd.target <= 0) {
            ProcessValidCandidates(dc_candidates, nd.invalid_dcs, evi);
        } else {
            ProcessInvalidCandidates(dc_candidates, nd.invalid_dcs, evi, nd.e + 1, nd.target);
        }
    }

    void ProcessValidCandidates(DCCandidateTrie& dc_candidates,
                                std::vector<DCCandidate> const& invalid_dcs,
                                PredicateBitset const& evi) {
        dc_candidates.ForEach([this](DCCandidate const& dc) { approx_covers_.Add(dc); });
        for (auto const& invalid_dc : invalid_dcs) {
            boost::dynamic_bitset<> can_add = invalid_dc.cand & (~evi);
            for (size_t i = can_add.find_first(); i != boost::dynamic_bitset<>::npos;
                 i = can_add.find_next(i)) {
                DCCandidate valid_dc{.bitset = invalid_dc.bitset};
                valid_dc.bitset.set(i);
                if (!approx_covers_.ContainsSubset(valid_dc)) approx_covers_.Add(valid_dc);
            }
        }
    }

    void ProcessInvalidCandidates(DCCandidateTrie& dc_candidates,
                                  std::vector<DCCandidate> const& invalid_dcs,
                                  PredicateBitset const& evi, size_t e, int64_t target) {
        for (auto const& invalid_dc : invalid_dcs) {
            boost::dynamic_bitset<> can_add = invalid_dc.cand & (~evi);
            for (size_t i = can_add.find_first(); i != boost::dynamic_bitset<>::npos;
                 i = can_add.find_next(i)) {
                DCCandidate valid_dc = invalid_dc;
                valid_dc.bitset.set(i);
                valid_dc.cand &= (~mutex_map_[i]);
                if (!dc_candidates.ContainsSubset(valid_dc) &&
                    !approx_covers_.ContainsSubset(valid_dc)) {
                    if (valid_dc.cand.any()) {
                        dc_candidates.Add(valid_dc);
                    } else if (IsApproxCover(valid_dc.bitset, e, target)) {
                        approx_covers_.Add(valid_dc);
                    }
                }
            }
        }
    }

    bool IsApproxCover(boost::dynamic_bitset<> const& dc, size_t e, int64_t target) {
        if (target <= 0) {
            return true;
        }
        for (; e < evidences_.size(); ++e) {
            if (IsSubset(dc, evidences_[e].evidence)) continue;
            target -= evidences_[e].count;
            if (target <= 0) return true;
        }
        return false;
    }

    template <typename T, typename U>
    bool IsSubset(T const& bitset1, U const& bitset2) {
        size_t min_size = std::min(bitset1.size(), bitset2.size());
        for (size_t i = 0; i < min_size; ++i) {
            if (bitset1.test(i) && !bitset2.test(i)) {
                return false;
            }
        }
        for (size_t i = min_size; i < bitset1.size(); ++i) {
            if (bitset1.test(i)) {
                return false;
            }
        }
        return true;
    }
};

}  // namespace algos::fastadc
