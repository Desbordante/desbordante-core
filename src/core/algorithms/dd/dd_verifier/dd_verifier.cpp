#include "dd_verifier.h"

#include <utility>

#include "descriptions.h"
#include "names.h"
#include "option_using.h"
#include "tabular_data/input_table/option.h"

#include <easylogging++.h>

#include "table/vertical.h"

namespace algos::dd {
    DDVerifier::DDVerifier() : Algorithm({}) {
        RegisterOptions();
        MakeOptionsAvailable({config::kTableOpt.GetName(), config::names::kDDString});
    }

    void DDVerifier::RegisterOptions() {
        DESBORDANTE_OPTION_USING;
        const auto default_dd = DDs();
        RegisterOption(config::kTableOpt(&input_table_));
        RegisterOption(Option{&dd_, kDDString, kDDDString, default_dd});
    }

    double DDVerifier::CalculateDistance(const model::ColumnIndex column_index,
                                         const std::pair<std::size_t, std::size_t> &tuple_pair) const {
        model::TypedColumnData const &column = typed_relation_->GetColumnData(column_index);
        const model::TypeId type_id = column.GetTypeId();

        if (type_id == +model::TypeId::kUndefined) {
            throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                        "\" type undefined.");
        }
        if (type_id == +model::TypeId::kMixed) {
            throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                        "\" contains values of different types.");
        }
        if (column.IsNull(tuple_pair.first) || column.IsNull(tuple_pair.second)) {
            throw std::runtime_error("Some of the value coordinates are nulls.");
        }
        if (column.IsEmpty(tuple_pair.first) || column.IsEmpty(tuple_pair.second)) {
            throw std::runtime_error("Some of the value coordinates are empty.");
        }
        double dif = 0;
        if (column.GetType().IsMetrizable()) {
            std::byte const *first_value = column.GetValue(tuple_pair.first);
            std::byte const *second_value = column.GetValue(tuple_pair.second);
            auto const &type = static_cast<model::IMetrizableType const &>(column.GetType());
            dif = type.Dist(first_value, second_value);
        }
        return dif;
    }

    std::vector<std::pair<int, int> > DDVerifier::GetRowsWhereLhsHolds(
        const std::list<model::DFStringConstraint> &constraints) const {
        LOG(INFO) << "DDVerifier::GetRowsHolds -- begin\n";
        std::vector<std::pair<int, int> > result;
        std::vector<model::ColumnIndex> columns;
        for (auto const &constraint: constraints) {
            model::ColumnIndex column_index = relation_->GetSchema()->GetColumn(constraint.column_name)->GetIndex();
            columns.push_back(column_index);
        }
        auto curr_constraint = constraints.begin();
        for (const auto column_index: columns) {
            if (result.empty()) {
                LOG(INFO) << "DDVerifier::GetRowsHolds -- columns.empty()\n ";
                for (size_t i = 0; i < num_rows_; i++) {
                    for (size_t j = i; j < num_rows_; j++) {
                        auto dif = CalculateDistance(column_index, {i, j});
                        LOG(INFO) << "Difference: " << dif;
                        if (dif <= curr_constraint->upper_bound && dif >= curr_constraint->lower_bound) {
                            result.emplace_back(i, j);
                        }
                    }
                }
            } else {
                for (std::size_t i = 0; i < result.size(); i++) {
                    LOG(INFO) << "DDVerifier::GetRowsHolds -- columns.else\n ";
                    if (const double dif = CalculateDistance(column_index, result[i]);
                        dif > curr_constraint->upper_bound || dif < curr_constraint->lower_bound) {
                        LOG(INFO) << "Difference: " << dif;
                        result.erase(result.begin() + i);
                        --i;
                    }
                }
            }
            ++curr_constraint;
        }
        return result;
    }

    void DDVerifier::LoadDataInternal() {
        relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, false);
        input_table_->Reset();
        typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, false);
    }

    void DDVerifier::MakeExecuteOptsAvailable() {
        using namespace config::names;
        MakeOptionsAvailable({kDDString});
    }

    unsigned long long DDVerifier::ExecuteInternal() {
        return 0;
    }

    void DDVerifier::CheckDFOnRhs(const std::vector<std::pair<int, int> > &lhs) {
        std::vector<model::ColumnIndex> columns;
        for (const auto &dd: dd_.right) {
            model::ColumnIndex column_index = relation_->GetSchema()->GetColumn(dd.column_name)->GetIndex();
            columns.push_back(column_index);
        }
        for (std::pair pair: lhs) {
            auto curr_constraint = dd_.right.cbegin();
            for (const auto column_index: columns) {
                if (const double dif = CalculateDistance(column_index, pair); !(
                    dif >= curr_constraint->lower_bound && dif <= curr_constraint->upper_bound)) {
                    std::pair<std::size_t, std::pair<int, int >> paie1 = {column_index, pair};
                    highlights_.emplace_back(paie1);
                    }
                ++curr_constraint;
            }
        }
    }


    bool DDVerifier::VerifyDD() {
        num_rows_ = typed_relation_->GetNumRows();
        num_columns_ = typed_relation_->GetNumColumns();
        const std::vector<std::pair<int, int> > lhs = GetRowsWhereLhsHolds(dd_.left);
        if (lhs.empty()) {
            return false;
        }
        CheckDFOnRhs(lhs);
        LOG(INFO) << "pairs_not_holds: " << highlights_.size() << ' ';
        for (auto pair: highlights_) {
            LOG(INFO) << "pair: " << pair.first << ' ' << pair.second << ' ';
        }
        return highlights_.empty();
        //TODO: this return and "LOGS" must be in function PrintStatistics and LOGS is a primitive realization of vizualization highlights
    }
}
