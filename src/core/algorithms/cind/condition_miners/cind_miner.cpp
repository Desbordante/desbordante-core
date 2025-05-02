#include "cind_miner.h"

#include "cind/condition_type.h"
#include "table/encoded_tables.h"
#include "timed_invoke.h"

namespace algos::cind {
CindMiner::CindMiner(config::InputTables& input_tables)
    : tables_(input_tables), condition_type_(CondType::group) {}

CindMiner::Attributes CindMiner::ClassifyAttributes(model::IND const& aind) const {
    Attributes result;
    for (auto const& column : tables_.GetTable(aind.GetLhs().GetTableIndex()).GetColumnData()) {
        auto const& ind_columns = aind.GetLhs().GetColumnIndices();
        auto const& ind_columns_rhs = aind.GetRhs().GetColumnIndices();
        if (std::find(ind_columns.cbegin(), ind_columns.cend(), column.GetColumn()->GetIndex()) !=
            ind_columns.cend()) {
            result.lhs_inclusion.push_back(&column);
        } else if (aind.GetLhs().GetTableIndex() == aind.GetRhs().GetTableIndex()) {
            if (std::find(ind_columns_rhs.cbegin(), ind_columns_rhs.cend(),
                          column.GetColumn()->GetIndex()) == ind_columns_rhs.cend()) {
                result.conditional.push_back(&column);
            }
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

unsigned long long CindMiner::Execute(std::list<model::IND> const& aind_list) {
    auto const execute = [&] {
        cind_collection_.Clear();
        for (auto const& aind : aind_list) {
            cind_collection_.Register(ExecuteSingle(aind));
        }
    };
    return util::TimedInvoke(execute);
}

std::vector<std::string> CindMiner::GetConditionalAttributesNames(
        AttrsType const& condition_attrs) {
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
