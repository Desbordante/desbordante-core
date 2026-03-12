#include "cind_miner.h"

#include "core/algorithms/cind/types.h"
#include "core/model/table/encoded_tables.h"
#include "core/util/timed_invoke.h"

namespace algos::cind {
CindMiner::CindMiner(config::InputTables& input_tables)
    : tables_(input_tables), condition_type_(CondType::group) {}

CindMiner::Attributes CindMiner::ClassifyAttributes(model::IND const& aind) const {
    Attributes result;

    auto const& lhs = aind.GetLhs();
    auto const& rhs = aind.GetRhs();

    auto const& lhs_indices = lhs.GetColumnIndices();
    auto const& rhs_indices = rhs.GetColumnIndices();

    auto const lhs_table = lhs.GetTableIndex();
    auto const rhs_table = rhs.GetTableIndex();
    bool const same_table = (lhs_table == rhs_table);

    for (auto const& column : tables_.GetTable(lhs_table).GetColumnData()) {
        auto const col_index = column.GetColumn()->GetIndex();

        if (std::ranges::find(lhs_indices, col_index) != lhs_indices.end()) {
            result.lhs_inclusion.push_back(&column);
            continue;
        }

        if (!same_table) {
            result.conditional.push_back(&column);
            continue;
        }

        if (std::ranges::find(rhs_indices, col_index) == rhs_indices.end()) {
            result.conditional.push_back(&column);
        }
    }

    for (auto const& column : tables_.GetTable(rhs_table).GetColumnData()) {
        auto const col_index = column.GetColumn()->GetIndex();
        if (std::ranges::find(rhs_indices, col_index) != rhs_indices.end()) {
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

    std::vector<std::string> result;
    result.reserve(condition_attrs.size());
    for (auto const* col_data : condition_attrs) {
        auto const* col = col_data->GetColumn();
        result.emplace_back(col->GetSchema()->GetName() + "." + col->GetName());
    }
    return result;
}
}  // namespace algos::cind
