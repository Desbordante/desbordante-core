#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier_base.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include "core/algorithms/pac/domain_pac.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/util/make_tuples.h"
#include "core/util/bitset_utils.h"
#include "core/util/logger.h"

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
}

void DomainPACVerifierBase::PreparePACTypeData() {
    original_value_tuples_ =
            pac::util::MakeTuples(TypedRelation().GetColumnData(), column_indices_);

    dists_from_domain_ = {};
    dists_from_domain_.reserve(original_value_tuples_->size());
    for (auto it = original_value_tuples_->begin(); it != original_value_tuples_->end(); ++it) {
        dists_from_domain_.emplace_back(it, domain_->DistFromDomain(*it));
    }
    std::ranges::sort(dists_from_domain_, {}, [](auto const& p) { return p.second; });
}

std::vector<std::pair<double, double>> DomainPACVerifierBase::FindEpsilons() const {
    auto total_tuples_num = original_value_tuples_->size();
    // Tuples number needed to satisfy min_delta
    std::size_t min_tuples_num = MinDelta() * total_tuples_num;
    std::size_t tuples_step;
    if (DeltaSteps() <= 1) {
        tuples_step = total_tuples_num - min_tuples_num;
    } else {
        tuples_step = static_cast<double>(total_tuples_num - min_tuples_num) / (DeltaSteps() - 1);
    }
    if (tuples_step == 0) {
        // min_tuples ~ total_tuples => make a few iterations
        tuples_step = 1;
    }
    LOG_TRACE("Min tuples num: {}, total tuples num: {}, tuples step: {}, delta steps: {}",
              min_tuples_num, total_tuples_num, tuples_step, DeltaSteps());

    auto end = domain_end_;
    auto domain_size = std::distance(dists_from_domain_.begin(), end);
    std::vector<std::pair<double, double>> result;
    std::size_t curr_size = domain_size;

    result.emplace_back(0, static_cast<double>(domain_size) / total_tuples_num);

    for (auto needed_tuples_num = min_tuples_num; needed_tuples_num <= total_tuples_num;
         needed_tuples_num += tuples_step) {
        if (needed_tuples_num < curr_size) {
            continue;
        }

        // Find eps_i
        auto need_to_add = needed_tuples_num - curr_size;
        auto actually_add =
                std::min(need_to_add,
                         static_cast<std::size_t>(std::distance(end, dists_from_domain_.end())));
        std::advance(end, actually_add);
        curr_size += actually_add;
        auto eps_i = end == dists_from_domain_.begin() ? 0 : std::prev(end)->second;
        LOG_TRACE("Eps for {} tuples: {}", needed_tuples_num, eps_i);

        // Refine delta_i
        while (end != dists_from_domain_.end() &&
               end->second - eps_i < PACVerifier::kDistThreshold) {
            std::advance(end, 1);
            ++curr_size;
        }
        assert(curr_size ==
               static_cast<std::size_t>(std::distance(dists_from_domain_.begin(), end)));
        LOG_TRACE("Refined size: {}", curr_size);
        auto delta_i = static_cast<double>(curr_size) / total_tuples_num;

        result.emplace_back(eps_i, delta_i);
    }
    return result;
}

void DomainPACVerifierBase::PACTypeExecuteInternal() {
    std::ostringstream oss;
    oss << '{';
    for (auto it = column_indices_.begin(); it != column_indices_.end(); ++it) {
        if (it != column_indices_.begin()) {
            oss << ", ";
        }
        oss << *it;
    }
    oss << '}';
    LOG_INFO("Verifying Domain PAC on columns {} with domain {}", oss.str(), domain_->ToString());

    domain_end_ = std::ranges::partition_point(
            dists_from_domain_, [](auto const& p) { return std::abs(p.second) < kDistThreshold; });

    LOG_TRACE("Distnaces from domain:");
    for ([[maybe_unused]] auto const& [it, dist] : dists_from_domain_) {
        LOG_TRACE("\t{}: {}", tuple_type_->ValueToString(*it), dist);
    }
    LOG_TRACE("Initial domain size: {}", std::distance(dists_from_domain_.cbegin(), domain_end_));

    auto empirical_probabilities = FindEpsilons();
    auto [eps, delta] = FindEpsilonDelta(std::move(empirical_probabilities));

    auto const schema = TypedRelation().GetSharedPtrSchema();
    MakePAC<model::DomainPAC>(
            schema, eps, delta, domain_,
            schema->GetVertical(util::IndicesToBitset(column_indices_, schema->GetNumColumns())));

    LOG_INFO("Result: {}", GetPAC().ToLongString());
}

DomainPACHighlight DomainPACVerifierBase::GetHighlights(double eps_1, double eps_2) const {
    if (eps_2 < 0) {
        eps_2 = GetPAC().GetEpsilon();
    }

    if (eps_2 <= eps_1) {
        return DomainPACHighlight{tuple_type_, original_value_tuples_, std::vector<TuplesIter>{}};
    }

    auto first_end = std::ranges::upper_bound(domain_end_, dists_from_domain_.end(), eps_1, {},
                                              [](auto const& p) { return p.second; });
    auto second_end = std::ranges::upper_bound(domain_end_, dists_from_domain_.end(), eps_2, {},
                                               [](auto const& p) { return p.second; });

    std::vector<TuplesIter> highlighted_tuples(std::distance(first_end, second_end));
    std::ranges::transform(first_end, second_end, highlighted_tuples.begin(),
                           [](auto const& p) { return p.first; });
    return DomainPACHighlight{tuple_type_, original_value_tuples_, std::move(highlighted_tuples)};
}
}  // namespace algos::pac_verifier
