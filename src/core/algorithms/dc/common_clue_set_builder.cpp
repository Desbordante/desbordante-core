#include <easylogging++.h>

#include "common_clue_set_builder.h"

#include "index_provider.h"
#include "operator.h"
#include "index_provider.h"

namespace model {

// Define static members
std::vector<PredicatePack> CommonClueSetBuilder::str_single_packs_;
std::vector<PredicatePack> CommonClueSetBuilder::str_cross_packs_;
std::vector<PredicatePack> CommonClueSetBuilder::num_single_packs_;
std::vector<PredicatePack> CommonClueSetBuilder::num_cross_packs_;
std::vector<Clue> CommonClueSetBuilder::correction_map_;

Clue CommonClueSetBuilder::BuildCorrectionMask(PredicatesSpan group,
                                               std::initializer_list<OperatorType>& types) {
    Clue mask;

    for (auto& p : group) {
        if (std::any_of(types.begin(), types.end(),
                        [p](OperatorType type) { return p->GetOperator() == type; })) {
            auto index = PredicateIndexProvider::GetInstance()->GetIndex(p);

            if (index < mask.size()) {
                mask.set(index);
            } else {
                throw std::runtime_error(
                        "Predicate index exceeds the size of ClueBitset, "
                        "such amount of predicates is not supported.");
            }
        }
    }

    return mask;
}

// TODO: no, just accept lambda by typaname Func with &&, cast to std::function is costly
void CommonClueSetBuilder::BuildPacksAndCorrectionMap(PredicatesVector const& predicates,
                                                      size_t group_size, PackAction action,
                                                      std::vector<PredicatePack>& pack,
                                                      size_t& count) {
    assert(predicates.size() % group_size == 0);
    size_t num_groups = predicates.size() / group_size;

    for (size_t i = 0; i < num_groups; ++i) {
        size_t base_index = i * group_size;
        PredicatesSpan group_span(predicates.begin() + base_index, group_size);

        action(group_span, pack, count);
    }
}

void CommonClueSetBuilder::BuildNumPacks(PredicatesVector const& predicates,
                                         std::vector<PredicatePack>& pack, size_t& count) {
    PackAction action = [](PredicatesSpan group_span, std::vector<PredicatePack>& pack,
                           size_t& count) {
        PredicatePtr eq = GetPredicateByType(group_span, OperatorType::kEqual);
        PredicatePtr gt = GetPredicateByType(group_span, OperatorType::kGreater);
        static std::initializer_list<OperatorType> eq_list = {
                OperatorType::kEqual, OperatorType::kUnequal, OperatorType::kLess,
                OperatorType::kGreaterEqual};
        static std::initializer_list<OperatorType> gt_list = {
                OperatorType::kLess, OperatorType::kGreater, OperatorType::kLessEqual,
                OperatorType::kGreaterEqual};

        correction_map_[count] = BuildCorrectionMask(group_span, eq_list);
        correction_map_[count + 1] = BuildCorrectionMask(group_span, gt_list);

        pack.emplace_back(eq, count, gt, count + 1);
        count += 2;
    };

    BuildPacksAndCorrectionMap(predicates, 6, action, pack, count);
}

void CommonClueSetBuilder::BuildCatPacks(PredicatesVector const& predicates,
                                         std::vector<PredicatePack>& pack, size_t& count) {
    PackAction action = [](PredicatesSpan group_span, std::vector<PredicatePack>& pack,
                           size_t& count) {
        PredicatePtr eq = GetPredicateByType(group_span, OperatorType::kEqual);
        static std::initializer_list<OperatorType> eq_list = {OperatorType::kEqual,
                                                              OperatorType::kUnequal};

        correction_map_[count] = BuildCorrectionMask(group_span, eq_list);

        pack.emplace_back(eq, count);
        count++;
    };

    BuildPacksAndCorrectionMap(predicates, 2, action, pack, count);
}

void CommonClueSetBuilder::BuildPredicatePacksAndCorrectionMap(PredicateBuilder const& pBuilder) {
    auto const& cat_single = pBuilder.GetStrSingleColumnPredicates();
    auto const& cat_cross = pBuilder.GetStrCrossColumnPredicates();
    auto const& num_single = pBuilder.GetNumSingleColumnPredicates();
    auto const& num_cross = pBuilder.GetNumCrossColumnPredicates();
    size_t count = 0;

    /*
     * The size of the correction map is calculated based on the number of categorical and
     * numerical predicates.
     * Categorical predicates are represented by 1 bit per pair of attributes, so we divide by 2.
     * Numerical predicates are represented by 2 bits per pair of attributes, and there are 6
     * possible numerical predicates, so we divide by 6 and multiply by 2 to account for the extra
     * bits.
     */
    correction_map_.resize(cat_single.size() / 2 + cat_cross.size() / 2 +
                           2 * num_single.size() / 6 + 2 * num_cross.size() / 6);

    BuildCatPacks(cat_single, str_single_packs_, count);
    BuildCatPacks(cat_cross, str_cross_packs_, count);
    BuildNumPacks(num_single, num_single_packs_, count);
    BuildNumPacks(num_cross, num_cross_packs_, count);

    LOG(DEBUG) << "  [CLUE] # of bits in clue: " << count;
    assert(count <= 64);
}

}  // namespace model
