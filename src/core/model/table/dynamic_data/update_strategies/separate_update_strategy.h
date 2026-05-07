#include <ranges>
#include <unordered_set>

#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/dynamic_data/dynamic_table_data.h"
#include "core/model/table/dynamic_data/update_strategies/update_strategy.h"
#include "core/model/table/idataset_stream.h"
#include "core/util/logger.h"

namespace model {

class SeparateUpdateStrategy : public IUpdateStrategy {
private:
    config::InputTable insert_data_;
    config::InputTable update_data_;
    std::unordered_set<size_t> const& delete_data_;

    void ProcessUpdate(IDynamicTableData* table) {
        if (update_data_ == nullptr) return;
        while (update_data_->HasNextRow()) {
            std::vector<std::string> row = update_data_->GetNextRow();
            if (row.size() != table->GetNumCols() + 1) {
                LOG_DEBUG("Got update table row with size: {}, skipping...", row.size());
                continue;
            }
            size_t row_id = std::stoull(row.front());
            std::vector<std::string> new_data(row.begin() + 1, row.end());
            table->UpdateRow(row_id, new_data);
        }
    }

    void ProcessDelete(IDynamicTableData* table) {
        std::ranges::for_each(delete_data_, [&table](size_t index) { table->DeleteRow(index); });
    }

    void ProcessInsert(IDynamicTableData* table) {
        // LOG_DEBUG("Process Inserting");
        assert(table);
        if (insert_data_ == nullptr) return;
        while (insert_data_->HasNextRow()) {
            // LOG_DEBUG("Inserting row");
            std::vector<std::string> row = insert_data_->GetNextRow();
            if (row.size() != table->GetNumCols()) {
                LOG_DEBUG("Got insert table row with size: {}, skipping...", row.size());
                continue;
            }
            table->AppendRow(row);
        }
        // LOG_DEBUG("Inserting Finish");
    }

public:
    explicit SeparateUpdateStrategy(config::InputTable insert_data, config::InputTable update_data,
                                    std::unordered_set<size_t> const& delete_data)
        : insert_data_(insert_data), update_data_(update_data), delete_data_(delete_data) {}

    void Update(IDynamicTableData* table) override {
        assert(table);
        ProcessDelete(table);
        ProcessInsert(table);
        ProcessUpdate(table);
    }
};

}  // namespace model