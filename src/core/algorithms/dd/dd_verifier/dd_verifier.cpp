#include "dd_verifier.h"

#include <utility>
#include <utility>
#include <boost/mpl/pair.hpp>

#include "descriptions.h"
#include "names.h"
#include "option_using.h"
#include "tabular_data/input_table/option.h"

namespace algos::dd {
    DDVerifier::DDVerifier(DD dd) : Algorithm({}), dd_(std::move(dd)) {
        RegisterOptions();
        MakeOptionsAvailable({config::kTableOpt.GetName()});
    }

    void DDVerifier::RegisterOptions() {
        DESBORDANTE_OPTION_USING;
        config::InputTable default_table;
        RegisterOption(config::kTableOpt(&input_table_));
    }

    //question -- Can I copy this func from Split algorithm
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

    std::vector<std::pair<int, int> > DDVerifier::GetRowsHolds(const std::list<model::DFStringConstraint> &constraints) const {
        std::vector<std::pair<int, int> > result;
        std::vector<model::ColumnIndex> columns;
        for (auto const &constraint: constraints) {
            model::ColumnIndex column_index = relation_->GetSchema()->GetColumn(constraint.column_name)->GetIndex();
            columns.push_back(column_index);
        }
        auto curr_constraint = constraints.cbegin();
        for (const auto column_index: columns) {
            const std::shared_ptr<model::PLI const> pli =
                    relation_->GetColumnData(column_index).GetPliOwnership();
            std::deque<model::PLI::Cluster> const &index = pli->GetIndex();
            if (result.empty()) {
                for (std::size_t i = 0; i < index.size(); i++) {
                    for (std::size_t j = i; j < index.size(); j++) {
                        int first_index = index[i][0];
                        int second_index = index[j][0];
                        if (const double dif = CalculateDistance(column_index, {first_index, second_index});
                            dif <= curr_constraint->upper_bound && dif >= curr_constraint->lower_bound) {
                            result.emplace_back(first_index, second_index);
                            }
                        ++curr_constraint;
                    }
                }
            } else {
                for (std::size_t i = 0; i < result.size(); i++) {
                    if (const double dif = CalculateDistance(column_index, result[i]);
                        dif > curr_constraint->upper_bound || dif < curr_constraint->lower_bound) {
                        result.erase(result.begin() + i);
                        i--;
                        }
                    ++curr_constraint;
                }
            }
        }
        return result;
    }

    void DDVerifier::LoadDataInternal() {
        relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, false); // nulls are
        // ignored
        input_table_->Reset();
        typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_,
                                                                           false); // nulls are ignored
    }

    void DDVerifier::MakeExecuteOptsAvailable() {
        using namespace config::names;
        MakeOptionsAvailable({kNumRows, kNumColumns});
    }

    std::vector<std::pair<int, int>> DDVerifier::CheckDFOnRhs(const std::vector<std::pair<int, int> >& lhs) const {
        std::vector<model::ColumnIndex> columns;
        for (const auto& dd : dd_.right) {
            model::ColumnIndex column_index = relation_->GetSchema()->GetColumn(dd.column_name)->GetIndex();
            columns.push_back(column_index);
        }
        std::vector<std::pair<int, int>> pairs_not_holds;
        auto curr_constraint = dd_.right.cbegin();
        for ( auto pair: lhs){
            for (const auto column_index : columns) {
                if (const double dif = CalculateDistance(column_index, pair); !(dif >= curr_constraint->lower_bound && dif <= curr_constraint->upper_bound)) {
                    pairs_not_holds.emplace_back(pair);
                }
                ++curr_constraint;
            }
        }
        return pairs_not_holds;
    }


    bool DDVerifier::VerifyDD() const {
        const std::vector<std::pair<int, int>> lhs = GetRowsHolds(dd_.left);
        const std::vector<std::pair<int, int>> pairs_not_holds = CheckDFOnRhs(lhs);
        return pairs_not_holds.empty();
    }
}
