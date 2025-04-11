#include "cind_miner.h"

#include "cind/condition_type.h"
#include "table/encoded_tables.h"

namespace algos::cind {
CindMiner::CindMiner(config::InputTables& input_tables)
    : tables_(input_tables), condition_type_(CondType::group) {}

CindMiner::Attributes CindMiner::ClassifyAttributes(model::IND const& aind) const {
    Attributes result;
    for (auto const& column : tables_.GetTable(aind.GetLhs().GetTableIndex()).GetColumnData()) {
        auto const& ind_columns = aind.GetLhs().GetColumnIndices();
        if (std::find(ind_columns.cbegin(), ind_columns.cend(), column.GetColumn()->GetIndex()) !=
            ind_columns.cend()) {
            result.lhs_inclusion.push_back(&column);
        } else {
            result.conditional.push_back(&column);
        }
    }
    for (auto const& column : tables_.GetTable(aind.GetRhs().GetTableIndex()).GetColumnData()) {
        auto const& ind_columns = aind.GetRhs().GetColumnIndices();
        if (std::find(ind_columns.cbegin(), ind_columns.cend(), column.GetColumn()->GetIndex()) !=
            ind_columns.cend()) {
            result.rhs_inclusion.push_back(&column);
        }
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

    for (auto const& aind : aind_list) {
        cind_collection_.Register(ExecuteSingle(aind));
    }
}

std::vector<std::string> CindMiner::GetConditionalAttributesNames(AttrsType const& condition_attrs) {
    if (condition_attrs.empty()) {
        return {};
    }
    std::vector<std::string> result(condition_attrs.size(),
                                    condition_attrs.back()->GetColumn()->GetSchema()->GetName());
    for (size_t i = 0; i < result.size(); ++i) {
        result[i].append(".").append(condition_attrs[i]->GetColumn()->GetName());
    }
    return result;
}
}  // namespace algos::cind
