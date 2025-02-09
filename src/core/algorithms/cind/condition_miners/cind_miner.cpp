#include "cind_miner.hpp"

#include "cind/condition_type.hpp"
#include "table/encoded_tables.hpp"

namespace algos::cind {
CindMiner::CindMiner(config::InputTables& input_tables)
    : tables_(input_tables), condition_type_(CondType::group) {}

CindMiner::Attributes CindMiner::ClassifyAttributes(model::IND const& aind) const {
    Attributes result;
    auto process_column = [&](EncodedColumnData const& column, model::ColumnCombination const& cc,
                              AttrsType& inclusion_attrs_dst) {
        auto const& ind_columns = cc.GetColumnIndices();
        if (std::find(ind_columns.cbegin(), ind_columns.cend(), column.GetColumn()->GetIndex()) !=
            ind_columns.cend()) {
            inclusion_attrs_dst.push_back(&column);
        } else {
            result.conditional.push_back(&column);
        }
    };

    for (auto const& column : tables_.GetTable(aind.GetLhs().GetTableIndex()).GetColumnData()) {
        process_column(column, aind.GetLhs(), result.lhs_inclusion);
    }
    for (auto const& column : tables_.GetTable(aind.GetRhs().GetTableIndex()).GetColumnData()) {
        process_column(column, aind.GetRhs(), result.rhs_inclusion);
    }
    return result;
}

void CindMiner::Execute(std::list<model::IND> const& aind_list) {
    fprintf(stderr, "validity: %lf, completeness: %lf, type: %s\n", min_validity_,
            min_completeness_, condition_type_._to_string());
    for (auto const& table : tables_.GetTables()) {
        for (auto const& column : table->GetColumnData()) {
            fprintf(stderr, "table = %s, column = %s, column size: %zu\n",
                    table->GetSchema()->GetName().c_str(), column.GetColumn()->GetName().c_str(),
                    column.GetNumRows());
            fprintf(stderr, "column data: [");
            for (size_t i = 0; i < column.GetNumRows(); ++i) {
                fprintf(stderr, "'%d::%s', ", column.GetValue(i), column.GetStringValue(i).c_str());
            }
            fprintf(stderr, "]\n");
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
    ExecuteSingle(aind_list.front());
    // for (auto const& aind : aind_list) {
    //     ExecuteSingle(aind);
    // }
}
}  // namespace algos::cind