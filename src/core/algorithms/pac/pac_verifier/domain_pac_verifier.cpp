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
#include "pac/model/metrizable_tuple.h"
#include "table/typed_column_data.h"

namespace algos::pac_verifier {
void DomainPACVerifier::RegisterDomainPACOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option<config::IndicesType>(&column_indices_, kColumnIndices, kDColumnIndices));

    RegisterOption(Option(&custom_metric_, kMetric, kDDomainPACMetric, pac::model::Metric{}));
    RegisterOption(
            Option(&custom_comparer_, kComparer, kDDomainPACComparer, pac::model::Comparer{}));
    RegisterOption(Option(&first_str_, kFirst, kDDomainPACFirst));
    RegisterOption(Option(&last_str_, kLast, kDDOmainPACLast));
}

void DomainPACVerifier::MakePACTypeExecuteOptionsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kColumnIndices, kMetric, kComparer, kFirst, kLast});
}

void DomainPACVerifier::ProcessPACTypeOptions() {
    std::vector<model::Type const*> types;
    for (auto col_idx : column_indices_) {
        auto const& col_data = typed_relation_->GetColumnData(col_idx);
        types.push_back(&col_data.GetType());
    }

    if (first_str_.size() != column_indices_.size() || last_str_.size() != column_indices_.size()) {
        throw config::ConfigurationError(
                "Both lower and upper bounds must contain the same number of values as "
                "column indices");
    }
    std::vector<std::byte> first_data(types.size());
    std::vector<std::byte> last_data(types.size());
    for (std::size_t i = 0; i < types.size(); ++i) {
        auto const* type = types[i];
        type->ValueFromStr(&first_data[i], first_str_[i]);
        type->ValueFromStr(&last_data[i], last_str_[i]);
    }

    tuple_type_ = std::make_shared<pac::model::MetrizableTupleType>(
            std::move(types), std::move(custom_metric_), std::move(custom_comparer_));

    pac::model::Domain domain{tuple_type_, std::move(first_data), std::move(last_data)};

    auto const* rel_schema = typed_relation_->GetSchema();
    pac_ = std::make_shared<model::DomainPAC>(
            0, 0, std::move(domain),
            Vertical(rel_schema, rel_schema->IndicesToBitset(column_indices_)));
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
              [this](auto a, auto b) { return tuple_type_->Compare(*a, *b) < 0; });

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
    auto const& domain = dynamic_cast<model::DomainPAC const*>(pac_.get())->GetDomain();
    auto const& lower_bound = domain.GetFirst();
    auto const& upper_bound = domain.GetLast();

    // Step 1: find maximum range of values that fall into domain
    auto first_it = std::ranges::find_if(sorted_value_tuples_, [this, lower_bound](auto const it) {
        return tuple_type_->Compare(*it, lower_bound) > 0;
    });
    if (first_it == sorted_value_tuples_.end()) {
        // TODO(senichenkov): some special case (a-la +\infty)? (and -\infty 7 lines later)
        throw config::ConfigurationError(
                "Lower bound of domain is greater than each value in input table");
    }

    auto last_it = std::ranges::partition_point(
            first_it, sorted_value_tuples_.end(), [this, upper_bound](auto const it) {
                return tuple_type_->Compare(*it, upper_bound) <= 0;
            });
    std::advance(last_it, -1);

    first_value_it_ = first_it;
    last_value_it_ = last_it;

    // Step 2: repeatedly widen domain
    auto eps_step = (max_eps - min_eps) / eps_steps;
    for (auto eps = min_eps; eps < max_eps; eps += eps_step) {
        first_it =
                std::ranges::partition_point(sorted_value_tuples_.begin(), std::next(first_it),
                                             [this, lower_bound, eps](auto const it) {
                                                 return tuple_type_->Dist(*it, lower_bound) < eps;
                                             });
        std::advance(first_it, -1);

        last_it = std::ranges::partition_point(
                last_it, sorted_value_tuples_.end(), [this, upper_bound, eps](auto const it) {
                    return tuple_type_->Dist(*it, upper_bound) < eps ||
                           tuple_type_->Compare(*it, upper_bound) <= 0;
                });
        std::advance(last_it, -1);
        falls_into_domain.push_back(std::distance(first_it, last_it));
    }
    return falls_into_domain;
}
}  // namespace algos::pac_verifier
