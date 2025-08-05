#pragma once

#include <cassert>
#include <unordered_map>
#include <unordered_set>

#include "dc/FastADC/model/predicate_set.h"
#include "dc/FastADC/providers/predicate_provider.h"

namespace algos::fastadc {

class Closure {
private:
    PredicateSet start_;
    PredicateSet closure_;
    bool added_{false};

    // Group predicates by operator, e.g. grouped_[op] = [predicates with that op]
    std::unordered_map<Operator, PredicatesVector> grouped_;

    PredicateProvider* provider_;

public:
    explicit Closure(PredicateSet const& start, PredicateProvider* provider)
        : start_(start), closure_(start), provider_(provider) {
        assert(provider_ != nullptr);
    }

    bool Construct() {
        if (!AddInitialImplications()) {
            return false;
        }

        added_ = true;
        while (added_) {
            added_ = false;
            if (!TransitivityStep()) {
                return false;
            }
        }
        return true;
    }

    PredicateSet const& GetClosure() const {
        return closure_;
    }

private:
    /**
     * Gathers all immediate implications from the start set.
     * Adds them to closure_ and grouped_.
     */
    bool AddInitialImplications() {
        std::unordered_set<PredicatePtr> initial_additions;

        for (PredicatePtr p : start_) {
            PredicatesVector const& implications = p->GetImplications(provider_);
            initial_additions.insert(implications.begin(), implications.end());

            if (PredicatePtr sym = p->GetSymmetric(provider_); sym != nullptr) {
                PredicatesVector const& sym_implications = sym->GetImplications(provider_);
                initial_additions.insert(sym_implications.begin(), sym_implications.end());
            }
        }
        return AddAll(initial_additions);
    }

    /**
     * Performs one "step" of transitivity:
     * - Gathers implications from closure (including symmetric).
     * - Applies transitive inferences based on grouped predicates.
     * - Handles special cases for inequalities and equalities.
     * - Finally, adds them to the closure.
     */
    bool TransitivityStep() {
        std::unordered_set<PredicatePtr> additions;

        AddImplicationsAndSymmetric(closure_, additions);
        AddTransitiveInferences(additions);
        HandleInequalitiesAndEqualities(additions);

        return AddAll(additions);
    }

    /**
     * For each predicate in the given set, add its implications and its
     * symmetric's implications (if any) into `additions`.
     */
    void AddImplicationsAndSymmetric(PredicateSet const& source,
                                     std::unordered_set<PredicatePtr>& additions) {
        for (PredicatePtr p : source) {
            PredicatesVector const& imps = p->GetImplications(provider_);
            additions.insert(imps.begin(), imps.end());

            if (PredicatePtr sym = p->GetSymmetric(provider_); sym != nullptr) {
                PredicatesVector const& sym_imps = sym->GetImplications(provider_);
                additions.insert(sym_imps.begin(), sym_imps.end());
            }
        }
    }

    /**
     * Leverages the grouped_ map to find transitive relationships among predicates.
     * For example, if we have A->B and B->C in the closure, we add A->C.
     */
    void AddTransitiveInferences(std::unordered_set<PredicatePtr>& additions) {
        // For every operator, see what transitive operators can be applied
        for (auto const& [op, pred_list] : grouped_) {
            for (Operator op_trans : op.GetTransitives()) {
                // If there's no group for the transitive operator, skip
                auto it_trans_group = grouped_.find(op_trans);
                if (it_trans_group == grouped_.end()) {
                    continue;
                }

                PredicatesVector const& trans_pred_list = it_trans_group->second;
                for (PredicatePtr p1 : pred_list) {
                    for (PredicatePtr p2 : trans_pred_list) {
                        if (p1 == p2) {
                            continue;
                        }

                        // Transitive inference: A->B and B->C => A->C
                        if (p1->GetRightOperand() == p2->GetLeftOperand()) {
                            PredicatePtr new_pred = provider_->GetPredicate(
                                    op, p1->GetLeftOperand(), p2->GetRightOperand());
                            additions.insert(new_pred);
                        }

                        // Also consider reversed scenario: C->A and A->B => C->B
                        if (p2->GetRightOperand() == p1->GetLeftOperand()) {
                            PredicatePtr new_pred = provider_->GetPredicate(
                                    op, p2->GetLeftOperand(), p1->GetRightOperand());
                            additions.insert(new_pred);
                        }
                    }
                }
            }
        }
    }

    /**
     * Checks for special-case inferences with operators like "Unequal", "LessEqual" etc.
     * For instance, if we know "x != y" and "x <= y", then "x < y" holds.
     */
    void HandleInequalitiesAndEqualities(std::unordered_set<PredicatePtr>& additions) {
        // If we have "x != y" and also "x <= y", then we can infer "x < y"
        // Similarly, if we have "x != y" and "x >= y", infer "x > y"
        if (auto it_uneq = grouped_.find(OperatorType::kUnequal); it_uneq != grouped_.end()) {
            for (PredicatePtr p : it_uneq->second) {
                ColumnOperand left = p->GetLeftOperand();
                ColumnOperand right = p->GetRightOperand();
                PredicatePtr less_equal =
                        provider_->GetPredicate(OperatorType::kLessEqual, left, right);
                PredicatePtr less = provider_->GetPredicate(OperatorType::kLess, left, right);
                PredicatePtr greater_equal =
                        provider_->GetPredicate(OperatorType::kGreaterEqual, left, right);
                PredicatePtr greater = provider_->GetPredicate(OperatorType::kGreater, left, right);

                if (closure_.Contains(less_equal)) {
                    additions.insert(less);
                }
                if (closure_.Contains(greater_equal)) {
                    additions.insert(greater);
                }
            }
        }

        // If we have x <= y and x >= y, we can infer x == y
        if (auto it_leq = grouped_.find(OperatorType::kLessEqual); it_leq != grouped_.end()) {
            for (PredicatePtr p : it_leq->second) {
                ColumnOperand left = p->GetLeftOperand();
                ColumnOperand right = p->GetRightOperand();
                PredicatePtr greater_equal =
                        provider_->GetPredicate(OperatorType::kGreaterEqual, left, right);
                PredicatePtr equal = provider_->GetPredicate(OperatorType::kEqual, left, right);

                if (closure_.Contains(greater_equal)) {
                    additions.insert(equal);
                }
            }
        }
    }

    /**
     * Adds all given predicates to the closure if they're not already there.
     * Updates the grouped_ map and sets `added_` if any new predicate is inserted.
     * Also checks for conflicts with inversed predicate.
     */
    template <typename Container>
    bool AddAll(Container const& predicates) {
        for (PredicatePtr p : predicates) {
            if (closure_.Add(p)) {
                // If we add p successfully but inversed predicate is also in the closure,
                // that means there's a contradiction.
                if (closure_.Contains(p->GetInverse(provider_))) {
                    return false;
                }
                grouped_[p->GetOperator()].push_back(p);
                added_ = true;
            }
        }
        return true;
    }
};

}  // namespace algos::fastadc
