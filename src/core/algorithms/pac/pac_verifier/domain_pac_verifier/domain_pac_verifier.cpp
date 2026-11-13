#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_highlight.h"
#include "core/algorithms/pac/pac_verifier/options.h"
#include "core/algorithms/pac/pac_verifier/util/make_tuples.h"
#include "core/algorithms/pac/pac_verifier/util/validation_mode.h"
#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/indices/option.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/logger.h"

namespace algos::pac_verifier {

void DomainPACVerifier::MakeDomain() {
    std::vector<model::Type const*> types(column_indices_.size());
    auto const& col_data = typed_relation_->GetColumnData();
    std::ranges::transform(column_indices_, types.begin(), [&col_data](auto const col_idx) {
        return &col_data[col_idx].GetType();
    });

    domain_->SetTypes(std::move(types));
    tuple_type_ = domain_->GetTupleTypePtr();
}

std::vector<EpsilonDelta> DomainPACVerifier::FindEpsilons() const {
    auto total_tuples_num = original_value_tuples_->size();
    // Tuples number needed to satisfy min_delta
    std::size_t min_tuples_num = std::ceil(min_delta_ * total_tuples_num);
    std::size_t tuples_step;
    if (delta_steps_ <= 1) {
        tuples_step = total_tuples_num - min_tuples_num;
    } else {
        tuples_step = static_cast<double>(total_tuples_num - min_tuples_num) / (delta_steps_ - 1);
    }
    if (tuples_step == 0) {
        // min_tuples ~ total_tuples => make a few iterations
        tuples_step = 1;
    }
    LOG_DEBUG("Min tuples num: {}, total tuples num: {}, tuples step: {}", min_tuples_num,
              total_tuples_num, tuples_step);

    auto end = domain_end_;
    auto domain_size = std::distance(dists_from_domain_.begin(), end);
    std::vector<EpsilonDelta> result;
    std::size_t curr_size = domain_size;

    auto domain_only_delta = static_cast<double>(domain_size) / total_tuples_num;
    if (domain_only_delta > min_delta_ - kDistThreshold) {
        result.emplace_back(0, domain_only_delta);
    }

    for (auto needed_tuples_num = min_tuples_num; needed_tuples_num <= total_tuples_num;
         needed_tuples_num += tuples_step) {
        if (needed_tuples_num <= curr_size) {
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
        while (end != dists_from_domain_.end() && end->second - eps_i < kDistThreshold) {
            std::advance(end, 1);
            ++curr_size;
        }
        assert(curr_size ==
               static_cast<std::size_t>(std::distance(dists_from_domain_.begin(), end)));
        LOG_TRACE("Refined size: {}", curr_size);
        auto delta_i = static_cast<double>(curr_size) / total_tuples_num;

        result.emplace_back(eps_i, delta_i);
    }
    result.emplace_back(dists_from_domain_.back().second, 1);
    return result;
}

void DomainPACVerifier::ExecuteInternal() {
    pac_man_ = std::make_unique<PACMan>(
            min_epsilon_, max_epsilon_, min_delta_, 1, diagonal_threshold_,
            [this](double epsilon) { return GetEpsilonDeltaForEpsilon(epsilon); });

    LOG_DEBUG("Common PAC verifier options: delta: [{}, 1], eps: [{}, {}], delta steps: {}",
              min_delta_, min_epsilon_, max_epsilon_, delta_steps_);

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

    EpsilonDelta eps_delta;
    auto [mode, eps_override, delta_override] =
            SelectPACValidationMode(mode_, min_epsilon_, max_epsilon_, min_delta_, 1);
    switch (mode) {
        case PACValidationMode::kFindPac:
            eps_delta = FindPAC();
            break;
        case PACValidationMode::kFindEpsilon:
            if (delta_override) {
                delta_ = *delta_override;
            }
            eps_delta = FindEpsilon();
            break;
        case PACValidationMode::kFindDelta:
            if (eps_override) {
                epsilon_ = *eps_override;
            }
            eps_delta = FindDelta();
            break;
    }

    auto const schema = typed_relation_->GetSharedPtrSchema();
    pac_ = model::DomainPAC{
            eps_delta.epsilon, eps_delta.delta, domain_->ToString(),
            schema->GetVertical(util::IndicesToBitset(column_indices_, schema->GetNumColumns()))};
    LOG_INFO("Result: {}", pac_->ToLongString());
}

EpsilonDelta DomainPACVerifier::GetEpsilonDeltaForEpsilon(double epsilon) const {
    auto it = std::ranges::upper_bound(
            dists_from_domain_, epsilon, {},
            [](std::pair<TuplesIter, double> const& pair) { return pair.second; });
    if (it != dists_from_domain_.begin()) {
        std::advance(it, -1);
    }
    // Refine delta
    auto eps = it->second;
    while (it != dists_from_domain_.end() && it->second - eps < kDistThreshold) {
        std::advance(it, 1);
    }
    auto delta = static_cast<double>(std::distance(dists_from_domain_.begin(), it)) /
                 original_value_tuples_->size();
    return {eps, delta};
}

DomainPACVerifier::EpsilonDelta DomainPACVerifier::FindPAC() {
    domain_end_ = std::ranges::partition_point(
            dists_from_domain_, [](auto const& p) { return std::abs(p.second) < kDistThreshold; });

    LOG_TRACE("Distances from domain:");
    for ([[maybe_unused]] auto const& [it, dist] : dists_from_domain_) {
        LOG_TRACE("\t{}: {}", tuple_type_->ValueToString(*it), dist);
    }
    LOG_TRACE("Initial domain size: {}", std::distance(dists_from_domain_.cbegin(), domain_end_));

    auto empirical_probabilities = FindEpsilons();
    return pac_man_->FindEpsilonDelta(std::move(empirical_probabilities));
}

DomainPACVerifier::EpsilonDelta DomainPACVerifier::FindEpsilon() {
    LOG_DEBUG("Dists from domain size: {}, original value tuples size: {}",
              dists_from_domain_.size(), original_value_tuples_->size());
    std::size_t needed_pairs = std::ceil(dists_from_domain_.size() * Delta());
    if (needed_pairs > 0) {
        --needed_pairs;
    }
    auto epsilon = dists_from_domain_[needed_pairs].second;

    // Refine delta
    auto it = std::next(dists_from_domain_.begin(), needed_pairs);
    while (it != dists_from_domain_.end() && std::next(it)->second - epsilon < kDistThreshold) {
        std::advance(it, 1);
    }
    auto actual_pairs = std::distance(dists_from_domain_.begin(), it);
    auto redined_delta = static_cast<double>(actual_pairs) / dists_from_domain_.size();
    return {epsilon, std::max(Delta(), redined_delta)};
}

DomainPACVerifier::EpsilonDelta DomainPACVerifier::FindDelta() {
    if (!std::isfinite(Epsilon())) {
        return {dists_from_domain_.back().second, 1};
    }
    return GetEpsilonDeltaForEpsilon(Epsilon());
}

DomainPACHighlight DomainPACVerifier::GetHighlights(double eps_1, double eps_2) const {
    if (!pac_) {
        throw std::runtime_error("Execute must be called before GetHighlights");
    }

    if (eps_2 < 0) {
        eps_2 = pac_->GetEpsilon();
    }

    if (eps_2 <= eps_1) {
        return DomainPACHighlight{tuple_type_, original_value_tuples_, std::vector<TuplesIter>{}};
    }

    auto first_end = std::ranges::upper_bound(domain_end_, dists_from_domain_.end(), eps_1, {},
                                              [](auto const& p) { return p.second; });
    auto second_end = std::ranges::upper_bound(domain_end_, dists_from_domain_.end(), eps_2, {},
                                               [](auto const& p) { return p.second; });

    std::vector<TuplesIter> highlighted_tuples;
    highlighted_tuples.reserve(std::distance(first_end, second_end));
    std::ranges::transform(first_end, second_end, std::back_inserter(highlighted_tuples),
                           [](auto const& p) { return p.first; });
    return DomainPACHighlight{tuple_type_, original_value_tuples_, std::move(highlighted_tuples)};
}

void DomainPACVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kMinEpsilon, kDiagonalThreshold});
}

void DomainPACVerifier::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, true);
    input_table_->Reset();
    if (typed_relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: PAC validation is meaningless.");
    }

    MakeDomain();

    original_value_tuples_ =
            pac::util::MakeTuples(typed_relation_->GetColumnData(), column_indices_);

    dists_from_domain_ = {};
    dists_from_domain_.reserve(original_value_tuples_->size());
    for (auto it = original_value_tuples_->begin(); it != original_value_tuples_->end(); ++it) {
        dists_from_domain_.emplace_back(it, domain_->DistFromDomain(*it));
    }
    std::ranges::sort(dists_from_domain_, {}, [](auto const& p) { return p.second; });
}

DomainPACVerifier::DomainPACVerifier() {
    DESBORDANTE_OPTION_USING;

    auto domain_not_nullptr = [](std::shared_ptr<pac::model::IDomain> const& domain) {
        if (!domain) {
            throw config::ConfigurationError("Domain cannot be null");
        }
    };

    // Load-data options
    RegisterOption(
            config::kTableOpt(&input_table_).SetConditionalOpts({{nullptr, {kColumnIndices}}}));
    RegisterOption(config::IndicesOption{kColumnIndices, kDColumnIndices, nullptr}(
            &column_indices_, [this]() { return input_table_->GetNumberOfColumns(); }));
    RegisterOption(Option(&domain_, kDomain, kDDomain).SetValueCheck(domain_not_nullptr));
    MakeOptionsAvailable({config::kTableOpt.GetName(), kDomain});

    auto get_min_epsilon = [this]() { return min_epsilon_; };
    auto get_max_epsilon = [this]() { return max_epsilon_; };
    auto get_min_delta = [this]() { return min_delta_; };

    // Execute options
    RegisterOption(kMinEpsilonOption(&min_epsilon_).SetConditionalOpts({{nullptr, {kMaxEpsilon}}}));
    RegisterOption(MaxEpsilonOption(get_min_epsilon)(&max_epsilon_)
                           .SetConditionalOpts({{nullptr, {kMinDelta}}}));
    RegisterOption(MinDeltaOption(get_min_epsilon, get_max_epsilon)(&min_delta_)
                           .SetConditionalOpts({{nullptr, {kDeltaSteps}}}));
    RegisterOption(DeltaStepsOption(get_min_delta, []() { return 1; })(&delta_steps_));
    RegisterOption(kDiagonalThresholdOption(&diagonal_threshold_));
}
}  // namespace algos::pac_verifier
