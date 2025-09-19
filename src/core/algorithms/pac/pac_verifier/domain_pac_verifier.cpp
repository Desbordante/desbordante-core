#include "algorithms/pac/pac_verifier/domain_pac_verifier.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>

#include <easylogging++.h>

#include "descriptions.h"
#include "exceptions.h"
#include "names.h"
#include "option_using.h"
#include "pac/domain_pac.h"
#include "table/typed_column_data.h"

namespace algos::pac_verifier {
void DomainPACVerifier::RegisterDomainPACOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option(&column_indices_, kColumnIndices, kDColumnIndices));
    RegisterOption(Option(&domain_, kDomain, kDDomain));
}

void DomainPACVerifier::MakePACTypeExecuteOptionsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kColumnIndices, kDomain});
}

void DomainPACVerifier::ProcessPACTypeOptions() {
    std::vector<model::Type const*> types;
    for (auto col_idx : column_indices_) {
        auto const& col_data = typed_relation_->GetColumnData(col_idx);
        types.push_back(&col_data.GetType());
    }

    domain_->SetTypes(std::move(types));
	tuple_type_ = domain_->GetTupleTypePtr();

    auto const* rel_schema = typed_relation_->GetSchema();
    pac_ = std::make_shared<model::DomainPAC>(
            0, 0, domain_, Vertical(rel_schema, rel_schema->IndicesToBitset(column_indices_)));
}

void DomainPACVerifier::PreparePACTypeData() {
    std::vector<std::vector<std::byte const*> const*> columns_data;
    for (auto col_idx : column_indices_) {
        auto const& col_data = typed_relation_->GetColumnData(col_idx);
        columns_data.push_back(&col_data.GetData());
    }

    original_value_tuples_ = std::make_shared<Tuples>();
    for (std::size_t row_idx = 0; row_idx < typed_relation_->GetNumRows(); ++row_idx) {
        auto& tuple = original_value_tuples_->emplace_back();
        for (auto const* col_data : columns_data) {
            tuple.push_back((*col_data)[row_idx]);
        }
    }

    sorted_value_tuples_ = {};
    for (auto it = original_value_tuples_->begin(); it != original_value_tuples_->end(); ++it) {
        sorted_value_tuples_.push_back(it);
    }
    std::sort(sorted_value_tuples_.begin(), sorted_value_tuples_.end(),
              [this](auto a, auto b) { return tuple_type_->Less(*a, *b); });

#if 0
    std::ostringstream oss;
    oss << "Sorted values:\n";
    for (auto const& it : sorted_value_tuples_) {
        oss << '\t' << tuple_type_->ValueToString(*it) << '\n';
    }
    LOG(INFO) << oss.str();
#endif
}

std::vector<std::size_t> DomainPACVerifier::CountSatisfyingTuples(double min_eps, double max_eps,
                                                                  unsigned long eps_steps) {
    std::vector<std::size_t> falls_into_domain;

    // Step 1: find maximum range of values that fall into domain
    auto first_it = std::ranges::find_if(sorted_value_tuples_, [this](auto const it) {
        return domain_->DistFromDomain(*it) == 0;
    });
    if (first_it == sorted_value_tuples_.end()) {
        // TODO(senichenkov): some special case (a-la +\infty)? (and -\infty 7 lines later)
        throw config::ConfigurationError(
                "Lower bound of domain is greater than each value in input table");
    }

    auto last_it = std::ranges::partition_point(
            first_it, sorted_value_tuples_.end(),
            [this](auto const it) { return domain_->DistFromDomain(*it) > 0; });
    std::advance(last_it, -1);

    first_value_it_ = first_it;
    last_value_it_ = last_it;

    // Step 2: repeatedly widen domain
    auto eps_step = (max_eps - min_eps) / eps_steps;
    for (auto eps = min_eps; eps < max_eps; eps += eps_step) {
        first_it = std::ranges::partition_point(
                sorted_value_tuples_.begin(), std::next(first_it),
                [this, eps](auto const it) { return domain_->DistFromDomain(*it) < eps; });
        std::advance(first_it, -1);

        last_it = std::ranges::partition_point(
                last_it, sorted_value_tuples_.end(),
                [this, eps](auto const it) { return domain_->DistFromDomain(*it) < eps; });
        std::advance(last_it, -1);
        falls_into_domain.push_back(std::distance(first_it, last_it));
    }
    return falls_into_domain;
}
}  // namespace algos::pac_verifier
