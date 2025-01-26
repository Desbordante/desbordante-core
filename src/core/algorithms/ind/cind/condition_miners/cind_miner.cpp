#include "cind_miner.hpp"

#include "ind/cind/condition_type.hpp"

namespace algos::cind {
CindMiner::CindMiner(config::InputTables& input_tables) : condition_type_(CondType::row) {
    for (size_t table_id = 0; table_id < input_tables.size(); ++table_id) {
        input_tables[table_id]->Reset();
        tables_.push_back(ColumnEncodedRelationData::CreateFrom(*input_tables[table_id], table_id));
    }
}

void CindMiner::Execute(std::list<model::IND> const& aind_list) {
    fprintf(stderr, "validity: %lf, completeness: %lf, type: %s\n", precision_, recall_,
            condition_type_._to_string());
    for (auto const& table : tables_) {
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