#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_index.h"
#include "core/model/table/idataset_stream.h"
#include "core/model/transaction/input_format_type.h"
#include "core/model/transaction/itemset.h"
#include "core/model/transaction/transactional_input_format.h"

namespace model {

class TransactionalData {
private:
    std::vector<std::string> item_universe_;
    std::unordered_map<size_t, Itemset> transactions_;

    TransactionalData(std::vector<std::string> item_universe,
                      std::unordered_map<size_t, Itemset> transactions)
        : item_universe_(std::move(item_universe)), transactions_(std::move(transactions)) {}

    static std::unique_ptr<TransactionalData> CreateFromSingular(IDatasetStream& data_stream,
                                                                 size_t tid_col_index = 0,
                                                                 size_t item_col_index = 1);

    static std::unique_ptr<TransactionalData> CreateFromTabular(IDatasetStream& data_stream,
                                                                bool has_tid);

public:
    struct Params {
        config::InputTable input_table;
        InputFormatType input_format_type = InputFormatType::singular;
        ColumnIndex tid_column_index = 0;
        ColumnIndex item_column_index = 1;

        bool first_column_tid = false;
    };

    TransactionalData() = delete;

    TransactionalData(TransactionalData const&) = delete;
    TransactionalData& operator=(TransactionalData const&) = delete;

    TransactionalData(TransactionalData&& other) = default;
    TransactionalData& operator=(TransactionalData&& other) = default;

    [[nodiscard]] static std::unique_ptr<TransactionalData> CreateFrom(Params& params);

    std::vector<std::string> const& GetItemUniverse() const noexcept {
        return item_universe_;
    }

    std::unordered_map<size_t, Itemset> const& GetTransactions() const noexcept {
        return transactions_;
    }

    size_t GetUniverseSize() const noexcept {
        return item_universe_.size();
    }

    size_t GetNumTransactions() const noexcept {
        return transactions_.size();
    }
};

}  // namespace model
