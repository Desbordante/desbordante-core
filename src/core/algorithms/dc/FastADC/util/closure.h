#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../model/predicate_set.h"
#include "../providers/predicate_provider.h"

namespace algos::fastadc {

class Closure {
private:
    PredicateSet start_;
    PredicateSet closure_;
    bool added_{false};
    std::unordered_map<Operator, std::vector<PredicatePtr>> grouped_;
    PredicateProvider* provider_;

public:
    explicit Closure(PredicateSet const& start, PredicateProvider* provider)
        : start_(start), closure_(start.provider), provider_(provider) {
        assert(provider);
    }

    bool Construct() {
        // Add all implications and symmetric implications from the start set
        for (PredicatePtr p : start_) {
            if (!AddAll(p->GetImplications(provider_))) {
                return false;
            }
            if (p->GetSymmetric(provider_) != nullptr &&
                !AddAll(p->GetSymmetric(provider_)->GetImplications(provider_))) {
                return false;
            }
        }

        added_ = true;
        // Perform transitivity steps until no new predicates are added
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
    bool TransitivityStep() {
        std::unordered_set<PredicatePtr> additions;

        // Add implications and symmetric implications to additions
        std::for_each(closure_.begin(), closure_.end(), [&](PredicatePtr p) {
            if (p->GetSymmetric(provider_) != nullptr) {
                auto const& sym_implications =
                        p->GetSymmetric(provider_)->GetImplications(provider_);
                additions.insert(sym_implications.begin(), sym_implications.end());
            }
            auto const& implications = p->GetImplications(provider_);
            additions.insert(implications.begin(), implications.end());
        });

        for (auto const& [op, list] : grouped_) {
            for (Operator op_trans : op.GetTransitives()) {
                auto const p_trans_it = grouped_.find(op_trans);
                if (p_trans_it == grouped_.end()) continue;

                std::vector<PredicatePtr> const& p_trans = p_trans_it->second;

                for (PredicatePtr p : list) {
                    for (PredicatePtr p2 : p_trans) {
                        if (p == p2) continue;

                        // Transitive inference: A -> B; B -> C
                        if (p->GetRightOperand() == p2->GetLeftOperand()) {
                            PredicatePtr new_pred = provider_->GetPredicate(op, p->GetLeftOperand(),
                                                                            p2->GetRightOperand());
                            additions.insert(new_pred);
                        }

                        // Transitive inference: C -> A; A -> B
                        if (p2->GetRightOperand() == p->GetLeftOperand()) {
                            PredicatePtr new_pred = provider_->GetPredicate(
                                    op, p2->GetLeftOperand(), p->GetRightOperand());
                            additions.insert(new_pred);
                        }
                    }
                }
            }
        }

        // Handle special cases for operators
        auto const& uneq_list_it = grouped_.find(OperatorType::kUnequal);
        if (uneq_list_it != grouped_.end()) {
            for (PredicatePtr p : uneq_list_it->second) {
                if (closure_.Contains(provider_->GetPredicate(
                            OperatorType::kLessEqual, p->GetLeftOperand(), p->GetRightOperand()))) {
                    additions.insert(provider_->GetPredicate(
                            OperatorType::kLess, p->GetLeftOperand(), p->GetRightOperand()));
                }
                if (closure_.Contains(provider_->GetPredicate(OperatorType::kGreaterEqual,
                                                              p->GetLeftOperand(),
                                                              p->GetRightOperand()))) {
                    additions.insert(provider_->GetPredicate(
                            OperatorType::kGreater, p->GetLeftOperand(), p->GetRightOperand()));
                }
            }
        }

        auto const& leq_list_it = grouped_.find(OperatorType::kLessEqual);
        if (leq_list_it != grouped_.end()) {
            for (PredicatePtr p : leq_list_it->second) {
                if (closure_.Contains(provider_->GetPredicate(OperatorType::kGreaterEqual,
                                                              p->GetLeftOperand(),
                                                              p->GetRightOperand()))) {
                    additions.insert(provider_->GetPredicate(
                            OperatorType::kEqual, p->GetLeftOperand(), p->GetRightOperand()));
                }
            }
        }

        // Add all newly inferred predicates
        return AddAll(additions);
    }

    // Add all predicates to the closure and update groups
    // TODO: concept?
    template <typename Container>
    bool AddAll(Container const& predicates) {
        for (PredicatePtr p : predicates) {
            if (closure_.Add(p)) {
                if (closure_.Contains(p->GetInverse(provider_))) {
                    return false;  // Conflict detected
                }
                grouped_[p->GetOperator()].push_back(p);
                added_ = true;
            }
        }
        return true;
    }
};

}  // namespace algos::fastadc