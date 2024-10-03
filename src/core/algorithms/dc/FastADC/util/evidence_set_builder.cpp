#include "dc/evidence_set_builder.h"

#include "dc/index_provider.h"

namespace algos::fastadc {

void EvidenceSetBuilder::BuildCardinalityMask(PredicateBuilder const& pBuilder) {
    auto add_predicate_to_mask = [this](PredicatesSpan group_span,
                                        std::initializer_list<OperatorType> types) {
        for (auto& predicate : group_span) {
            if (std::any_of(types.begin(), types.end(), [&predicate](OperatorType type) {
                    return predicate->GetOperator() == type;
                })) {
                size_t index = PredicateIndexProvider::GetInstance()->GetIndex(predicate);
                if (index < cardinality_mask_.size()) {
                    cardinality_mask_.set(index);
                } else {
                    throw std::runtime_error("Predicate index exceeds the size of ClueBitset.");
                }
            }
        }
    };

    // Build mask for categorical single-column predicates (group size: 2)
    auto const& cat_single = pBuilder.GetStrSingleColumnPredicates();
    size_t group_size = 2;  // Categorical predicates have 2 predicates per group.
    for (size_t i = 0; i < cat_single.size(); i += group_size) {
        PredicatesSpan group_span(cat_single.begin() + i, group_size);
        add_predicate_to_mask(group_span, {OperatorType::kUnequal});
    }

    // Build mask for string cross-column predicates (group size: 2)
    auto const& cat_cross = pBuilder.GetStrCrossColumnPredicates();
    for (size_t i = 0; i < cat_cross.size(); i += group_size) {
        PredicatesSpan group_span(cat_cross.begin() + i, group_size);
        add_predicate_to_mask(group_span, {OperatorType::kUnequal});
    }

    // Build mask for numerical single-column predicates (group size: 6)
    auto const& num_single = pBuilder.GetNumSingleColumnPredicates();
    group_size = 6;  // Numerical predicates have 6 predicates per group.
    for (size_t i = 0; i < num_single.size(); i += group_size) {
        PredicatesSpan group_span(num_single.begin() + i, group_size);
        add_predicate_to_mask(group_span, {OperatorType::kUnequal, OperatorType::kLess,
                                           OperatorType::kLessEqual});
    }

    // Build mask for numerical cross-column predicates (group size: 6)
    auto const& num_cross = pBuilder.GetNumCrossColumnPredicates();
    for (size_t i = 0; i < num_cross.size(); i += group_size) {
        PredicatesSpan group_span(num_cross.begin() + i, group_size);
        add_predicate_to_mask(group_span, {OperatorType::kUnequal, OperatorType::kLess,
                                           OperatorType::kLessEqual});
    }
}

}  // namespace algos::fastadc
