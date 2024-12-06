#include "dd_verifier.h"

#include <utility>

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
        RegisterOption(Option{&num_rows_, kNumRows, kDNumRows, 0U});
        RegisterOption(Option{&num_columns_, kNumColumns, kDNUmColumns, 0U});
    }
    //question -- Can I copy this func from Split algorithm
    double DDVerifier::CalculateDistance(const model::ColumnIndex column_index,
                                         const std::pair<std::size_t, std::size_t> &tuple_pair) const {
        model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);
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
            std::byte const* first_value = column.GetValue(tuple_pair.first);
            std::byte const* second_value = column.GetValue(tuple_pair.second);
            auto const& type = static_cast<model::IMetrizableType const&>(column.GetType());
            dif = type.Dist(first_value, second_value);
        }
        return dif;
    }

    std::vector<model::PLI::Cluster> DDVerifier::GetRowsHolds(DF const constraint) {
        model::ColumnIndex column_index = relation_->GetSchema()->GetColumn(constraint.column_name)->GetIndex();
        std::shared_ptr<model::PLI const> pli =
                relation_->GetColumnData(column_index).GetPliOwnership();
        std::deque<model::PLI::Cluster> const& index = pli->GetIndex();
    }

    void DDVerifier::LoadDataInternal() {
        relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, false);  // nulls are
        // ignored
        input_table_->Reset();
        typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_,
                                                                           false);  // nulls are ignored
    }
    void DDVerifier::MakeExecuteOptsAvailable() {
        using namespace config::names;
        MakeOptionsAvailable({kNumRows, kNumColumns});
    }



    /*VerifyDD(){
     *get tuple of pairs, which holds lhs
     *Check this pairs on holding rhs{
     *if not hold{
     *break;
     *NOT HOLDS
     *}
     *}
     *return OK;
     *
     *}
     *
     *
     *
     */

}
