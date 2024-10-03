#pragma once

#include <vector>

#include "predicate_builder.h"

namespace algos::fastadc {

/* Maximum supported number of bits in clue is 64 */
using Clue = std::bitset<64>;
using ClueSet = std::unordered_map<Clue, int64_t>;

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

/**
 * Abstract class for building a clue set.
 *
 * This class contains static methods and data structures that will be used by derived classes.
 * The derived classes override BuildClueSet, since building ClueSet for a pair of same/different
 * columns is a didfferent task.
 *
 * A clue set is a compact representation of all satisfied predicates between
 * tuple pairs in a relation. The clues are built by evaluating the relationships
 * between tuples using Position List Index (PLI) data structures, and mapping
 * predicate results to bitsets.
 */
class CommonClueSetBuilder {
public:
    CommonClueSetBuilder(CommonClueSetBuilder const& other) = delete;
    CommonClueSetBuilder& operator=(CommonClueSetBuilder const& other) = delete;
    CommonClueSetBuilder(CommonClueSetBuilder&& other) noexcept = default;
    CommonClueSetBuilder& operator=(CommonClueSetBuilder&& other) noexcept = default;
    CommonClueSetBuilder() = default;

    virtual ClueSet BuildClueSet() = 0;

    virtual ~CommonClueSetBuilder() = default;

    /*
     * Static variable in a function is initialized only once and retains its
     * value across multiple invocations of the function. Here, is_configured
     * is of type bool, and it is initialized using a lambda function that is
     * immediately invoked using ().
     */
    static void ConfigureOnce(PredicateBuilder const& pbuilder) {
        static bool is_configured [[maybe_unused]] = [&]() {
            BuildPredicatePacksAndCorrectionMap(pbuilder);
            return true;
        }();
    }

    /* For tests */
    size_t GetNumberOfBitsInClue() const {
        return correction_map_.size();
    }

    /* For tests */
    std::vector<PredicatePack> GetPredicatePacks() const {
        std::vector<PredicatePack> packs;
        packs.insert(packs.end(), str_single_packs_.begin(), str_single_packs_.end());
        packs.insert(packs.end(), str_cross_packs_.begin(), str_cross_packs_.end());
        packs.insert(packs.end(), num_single_packs_.begin(), num_single_packs_.end());
        packs.insert(packs.end(), num_cross_packs_.begin(), num_cross_packs_.end());
        return packs;
    }

    /* For tests */
    static std::vector<PredicateBitset> GetCorrectionMap() {
        return correction_map_;
    }

private:
    /**
     * Builds predicate packs and the correction map from the provided PredicateBuilder.
     * Predicate packs
     */
    static void BuildPredicatePacksAndCorrectionMap(PredicateBuilder const& pBuilder);

    static PredicateBitset BuildCorrectionMask(PredicatesSpan group,
                                               std::initializer_list<OperatorType>& types);

    using PackAction = std::function<void(PredicatesSpan, std::vector<PredicatePack>&, size_t&)>;

    /**
     * Splits vector @predicates by groups of @group_size, applies @action to them
     * to add freshly created pack to @pack.
     *
     * Also populates correction_map_.
     */
    static void BuildPacksAndCorrectionMap(PredicatesVector const& predicates, size_t group_size,
                                           PackAction action, std::vector<PredicatePack>& pack,
                                           size_t& count);

    /**
     * Processes numerical single-column predicates from a flat list, where each group
     * of six predicates for one column pair (EQUAL, UNEQUAL, GREATER, LESS, GREATER_EQUAL,
     * LESS_EQUAL) is stored consecutively one after another, like
     * "flatten([[6 preds], [6 preds], ..., [6 preds]])"
     */
    static void BuildNumPacks(PredicatesVector const& predicates, std::vector<PredicatePack>& pack,
                              size_t& count);

    /**
     * Processes categorical single-column predicates from a flat list, where each group
     * of two predicates for one column pair (EQUAL, UNEQUAL) is stored consecutively
     * one after another, like "flatten([[2 preds], [2 preds], ..., [2 preds]])"
     */
    static void BuildCatPacks(PredicatesVector const& predicates, std::vector<PredicatePack>& pack,
                              size_t& count);

protected:
    template <typename... Vectors>
    ClueSet AccumulateClues(Vectors const&... vectors) const {
        ClueSet clue_set;
        auto insert_clues = [&clue_set](std::vector<Clue> const& clues) {
            for (auto const& clue : clues) {
                clue_set[clue]++;
            }
        };
        (insert_clues(vectors), ...);
        return clue_set;
    }

    /** String single-column predicate packs */
    static std::vector<PredicatePack> str_single_packs_;
    /** String cross-column predicate packs */
    static std::vector<PredicatePack> str_cross_packs_;
    /** Numerical single-column predicate packs */
    static std::vector<PredicatePack> num_single_packs_;
    /** Numerical cross-column predicate packs */
    static std::vector<PredicatePack> num_cross_packs_;
    /** Predicate id -> its correction mask */
    static std::vector<PredicateBitset> correction_map_;
};

}  // namespace algos::fastadc
