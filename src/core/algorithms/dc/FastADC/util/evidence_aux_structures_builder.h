#pragma once
#include <bitset>
#include <cstddef>
#include <functional>
#include <span>
#include <stdexcept>
#include <vector>

#include "common_clue_set_builder.h"
#include "dc/FastADC/model/column_operand.h"
#include "dc/FastADC/model/operator.h"
#include "dc/FastADC/model/predicate.h"
#include "dc/FastADC/providers/index_provider.h"
#include "predicate_builder.h"
#include "table/column.h"

namespace algos::fastadc {

/** Predicate EQ (and GT) of one column pair */
struct PredicatePack {
    PredicatePtr eq;
    PredicatePtr gt;
    size_t left_idx, right_idx;
    Clue eq_mask, gt_mask;

    PredicatePack(PredicatePtr eq, size_t eq_pos)
        : eq(eq),
          gt(nullptr),
          left_idx(eq->GetLeftOperand().GetColumn()->GetIndex()),
          right_idx(eq->GetRightOperand().GetColumn()->GetIndex()),
          eq_mask(0),
          gt_mask(0) {
        eq_mask.set(eq_pos);
    }

    PredicatePack(PredicatePtr eq, size_t eq_pos, PredicatePtr gt, size_t gt_pos)
        : eq(eq),
          gt(gt),
          left_idx(eq->GetLeftOperand().GetColumn()->GetIndex()),
          right_idx(eq->GetRightOperand().GetColumn()->GetIndex()),
          eq_mask(0),
          gt_mask(0) {
        eq_mask.set(eq_pos);
        gt_mask.set(gt_pos);
    }
};

struct PredicatePacks {
    /** String single-column predicate packs */
    std::vector<PredicatePack> str_single;
    /** String cross-column predicate packs */
    std::vector<PredicatePack> str_cross;
    /** Numerical single-column predicate packs */
    std::vector<PredicatePack> num_single;
    /** Numerical cross-column predicate packs */
    std::vector<PredicatePack> num_cross;
};

/**
 * Class with structures that helps to build an clue/evidence set.
 *
 * This class builds predicate packs, correction map and cardinality mask,
 * which will be used later to build clue and evidence sets
 *
 * A clue set is a compact representation of all satisfied predicates between
 * tuple pairs in a relation. The clues are built by evaluating the relationships
 * between tuples using Position List Index (PLI) data structures, and mapping
 * predicate results to bitsets.
 */
class EvidenceAuxStructuresBuilder {
public:
    EvidenceAuxStructuresBuilder(EvidenceAuxStructuresBuilder const& other) = delete;
    EvidenceAuxStructuresBuilder& operator=(EvidenceAuxStructuresBuilder const& other) = delete;
    EvidenceAuxStructuresBuilder(EvidenceAuxStructuresBuilder&& other) noexcept = default;
    EvidenceAuxStructuresBuilder& operator=(EvidenceAuxStructuresBuilder&& other) noexcept = delete;

    EvidenceAuxStructuresBuilder(PredicateBuilder const& pbuilder)
        : num_single_(pbuilder.GetNumSingleColumnPredicates()),
          num_cross_(pbuilder.GetNumCrossColumnPredicates()),
          str_single_(pbuilder.GetStrSingleColumnPredicates()),
          str_cross_(pbuilder.GetStrCrossColumnPredicates()),
          provider_(pbuilder.predicate_index_provider) {}

    /* For tests */
    size_t GetNumberOfBitsInClue() const {
        return correction_map_.size();
    }

    /* For tests */
    std::vector<PredicatePack> GetPredicatePacksAsOneVector() const {
        std::vector<PredicatePack> ret;
        ret.insert(ret.end(), packs_.str_single.begin(), packs_.str_single.end());
        ret.insert(ret.end(), packs_.str_cross.begin(), packs_.str_cross.end());
        ret.insert(ret.end(), packs_.num_single.begin(), packs_.num_single.end());
        ret.insert(ret.end(), packs_.num_cross.begin(), packs_.num_cross.end());
        return ret;
    }

    std::vector<PredicateBitset> const& GetCorrectionMap() const {
        return correction_map_;
    }

    PredicatePacks const& GetPredicatePacks() const {
        return packs_;
    }

    PredicateBitset const& GetCardinalityMask() const {
        return cardinality_mask_;
    }

    /**
     * Builds predicate packs, correction map and cardinality mask
     */
    void BuildAll();

private:
    template <std::size_t N>
    PredicateBitset BuildMask(PredicatesSpan group, OperatorType const (&types)[N]) {
        PredicateBitset mask;

        for (auto& p : group) {
            if (std::any_of(std::begin(types), std::end(types),
                            [p](OperatorType type) { return p->GetOperator() == type; })) {
                auto index = provider_->GetIndex(p);

                if (index < mask.size()) {
                    mask.set(index);
                } else {
                    throw std::runtime_error(
                            "Predicate index exceeds the size of PredicateBitset, "
                            "such amount of predicates is not supported.");
                }
            }
        }

        return mask;
    }

    using PackAction = std::function<void(PredicatesSpan)>;

    /**
     * Splits vector @predicates by groups of @group_size and applies @action to them.
     *
     * @actions should build predicate packs, correction map and cardinality mask
     */
    void BuildAll(PredicatesVector const& predicates, size_t group_size, PackAction action);

    /**
     * Processes numerical single-column predicates from a flat list, where each group
     * of six predicates for one column pair (EQUAL, UNEQUAL, GREATER, LESS, GREATER_EQUAL,
     * LESS_EQUAL) is stored consecutively one after another, like
     * "flatten([[6 preds], [6 preds], ..., [6 preds]])"
     */
    void ProcessNumPredicates(PredicatesVector const& predicates, std::vector<PredicatePack>& pack,
                              size_t& count);

    /**
     * Processes categorical single-column predicates from a flat list, where each group
     * of two predicates for one column pair (EQUAL, UNEQUAL) is stored consecutively
     * one after another, like "flatten([[2 preds], [2 preds], ..., [2 preds]])"
     */
    void ProcessCatPredicates(PredicatesVector const& predicates, std::vector<PredicatePack>& pack,
                              size_t& count);

    PredicatePacks packs_;
    /** Predicate id -> its correction mask */
    std::vector<PredicateBitset> correction_map_;
    PredicateBitset cardinality_mask_;

    PredicatesVector const& num_single_;
    PredicatesVector const& num_cross_;
    PredicatesVector const& str_single_;
    PredicatesVector const& str_cross_;
    PredicateIndexProvider* provider_;
};

}  // namespace algos::fastadc
