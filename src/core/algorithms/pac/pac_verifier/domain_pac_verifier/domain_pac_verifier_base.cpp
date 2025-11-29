#include "algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier_base.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <memory>

#include "logger.h"
#include "pac/domain_pac.h"
#include "pac/pac_verifier/domain_pac_verifier/domain_pac_highlight.h"
#include "util/bitset_utils.h"

namespace algos::pac_verifier {

void DomainPACVerifierBase::ProcessPACTypeOptions() {
    std::vector<model::Type const*> types;
    auto const& col_data = TypedRelation().GetColumnData();
    std::ranges::transform(
            column_indices_, std::back_inserter(types),
            [&col_data](auto const col_idx) { return &col_data[col_idx].GetType(); });

    domain_->SetTypes(std::move(types));
    domain_->SetDistFromNullIsInfinity(DistFromNullIsInfty());
    tuple_type_ = domain_->GetTupleTypePtr();

    auto const* rel_schema = TypedRelation().GetSchema();
    MakePAC<model::DomainPAC>(
            0, 0, domain_,
            Vertical(rel_schema,
                     util::IndicesToBitset(column_indices_, rel_schema->GetNumColumns())));
}

void DomainPACVerifierBase::PreparePACTypeData() {
    std::vector<std::vector<std::byte const*> const*> columns_data;
    auto const& col_data = TypedRelation().GetColumnData();
    std::ranges::transform(
            column_indices_, std::back_inserter(columns_data),
            [&col_data](auto const col_idx) { return &col_data[col_idx].GetData(); });

    original_value_tuples_ = std::make_shared<Tuples>();
    for (std::size_t row_idx = 0; row_idx < TypedRelation().GetNumRows(); ++row_idx) {
        auto& tuple = original_value_tuples_->emplace_back();
        std::ranges::transform(columns_data, std::back_inserter(tuple),
                               [row_idx](auto const* col_data) { return (*col_data)[row_idx]; });
    }

    sorted_value_tuples_ = {};
    for (auto it = original_value_tuples_->begin(); it != original_value_tuples_->end(); ++it) {
        sorted_value_tuples_.push_back(it);
    }
    std::ranges::sort(sorted_value_tuples_,
                      [this](auto a, auto b) { return tuple_type_->Less(*a, *b); });
}

std::vector<std::size_t> DomainPACVerifierBase::CountSatisfyingTuples() {
    std::vector<std::size_t> falls_into_domain;
    // Special cases: infinity -- domain \cap values = \emptyset
    bool minus_infty = false;

    // Step 1: find maximum range of values that fall into domain
    auto first_it = std::ranges::find_if(sorted_value_tuples_, [this](auto const it) {
        return domain_->DistFromDomain(*it) == 0;
    });

    auto after_last_it = std::ranges::partition_point(
            first_it, sorted_value_tuples_.end(),
            [this](auto const it) { return domain_->DistFromDomain(*it) == 0; });
    if (first_it == sorted_value_tuples_.end()) {
        if (domain_->DistFromDomain(*sorted_value_tuples_.front()) <
            domain_->DistFromDomain(*sorted_value_tuples_.back())) {
            minus_infty = true;
            first_it = after_last_it = sorted_value_tuples_.begin();
        }
        // else -- plus infinity
    }

    first_value_it_ = first_it;
    after_last_value_it_ = after_last_it;

    // Step 2: repeatedly widen domain
    auto eps_step = (MaxEpsilon() - MinEpsilon()) / (EpsilonSteps() - 1);
    for (std::size_t step = 0; step < EpsilonSteps(); ++step) {
        auto eps = MinEpsilon() + step * eps_step;

        if (first_it == sorted_value_tuples_.end()) {
            std::advance(first_it, -1);
        }
        if (!minus_infty) {
            first_it = std::ranges::partition_point(
                    sorted_value_tuples_.begin(), std::next(first_it),
                    [this, eps](auto const it) { return domain_->DistFromDomain(*it) > eps; });
        }

        after_last_it = std::ranges::partition_point(
                after_last_it, sorted_value_tuples_.end(),
                [this, eps](auto const it) { return domain_->DistFromDomain(*it) <= eps; });
        falls_into_domain.push_back(std::distance(first_it, after_last_it));
    }
    return falls_into_domain;
}

std::unique_ptr<PACHighlight> DomainPACVerifierBase::GetHighlightsInternal(double eps_1,
                                                                           double eps_2) const {
    if (eps_1 < 0) {
        eps_1 = MinEpsilon();
    }
    if (eps_2 < 0) {
        eps_2 = GetPAC().GetEpsilon();
    }

    if (eps_2 <= eps_1) {
        return std::make_unique<DomainPACHighlight>(tuple_type_, original_value_tuples_,
                                                    std::vector<TuplesIter>{});
    }

    auto first_1_it = std::ranges::partition_point(
            sorted_value_tuples_.begin(), std::next(first_value_it_),
            [this, eps_1](auto const it) { return domain_->DistFromDomain(*it) > eps_1; });
    auto first_2_it = std::ranges::partition_point(
            sorted_value_tuples_.begin(), first_1_it,
            [this, eps_2](auto const it) { return domain_->DistFromDomain(*it) > eps_2; });
    auto after_last_1_it = std::ranges::partition_point(
            std::prev(after_last_value_it_), sorted_value_tuples_.end(),
            [this, eps_1](auto const it) { return domain_->DistFromDomain(*it) <= eps_1; });
    auto after_last_2_it = std::ranges::partition_point(
            std::prev(after_last_1_it), sorted_value_tuples_.end(),
            [this, eps_2](auto const it) { return domain_->DistFromDomain(*it) <= eps_2; });

    std::vector<TuplesIter> highlighted_tuples(first_2_it, first_1_it);
    highlighted_tuples.insert(highlighted_tuples.end(), after_last_1_it, after_last_2_it);
    return std::make_unique<DomainPACHighlight>(tuple_type_, original_value_tuples_,
                                                std::move(highlighted_tuples));
}

unsigned long long DomainPACVerifierBase::ExecuteInternal() {
    auto start = std::chrono::system_clock::now();
    LOG_INFO("Verifying {}...", GetPAC().ToLongString());
    auto satisfying_tuples = CountSatisfyingTuples();

    std::vector<double> empirical_probabilities(satisfying_tuples.size());
    auto const num_rows = TypedRelation().GetNumRows();
    std::ranges::transform(satisfying_tuples, empirical_probabilities.begin(),
                           [num_rows](unsigned long const number) {
                               return static_cast<double>(number) / num_rows;
                           });
    auto [eps, delta] = FindEpsilonDelta(empirical_probabilities);
    GetPAC().SetEpsilon(eps);
    GetPAC().SetDelta(delta);

    LOG_INFO("Result: {}", GetPAC().ToLongString());
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now() - start)
                           .count();
    LOG_INFO("Validation took {}ms", elapsed);
    return elapsed;
}
}  // namespace algos::pac_verifier
