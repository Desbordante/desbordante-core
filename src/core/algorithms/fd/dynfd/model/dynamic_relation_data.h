#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "compressed_column_data.h"
#include "fd/hycommon/preprocessor.h"
#include "fd/raw_fd.h"
#include "model/table/idataset_stream.h"
#include "model/table/relation_data.h"
#include "table/vertical.h"
#include "tabular_data/input_table_type.h"

namespace algos::dynfd {

using CompressedRecord = std::vector<int>;
using CompressedRecords = std::vector<CompressedRecord>;
using CompressedRecordsPtr = std::shared_ptr<CompressedRecords>;

class DynamicRelationData : public AbstractRelationData<CompressedColumnData> {
    using ValueDictionary = std::vector<std::unordered_map<std::string, int>>;

    std::unordered_set<size_t> stored_row_ids_;
    ValueDictionary value_dictionary_;
    int next_value_id_;
    size_t next_record_id_;
    CompressedRecordsPtr compressed_records_;

private:
    [[nodiscard]] size_t GetNumRows() const final;

public:
    explicit DynamicRelationData(std::unique_ptr<RelationalSchema> schema,
                                 std::vector<ColumnType> column_data,
                                 std::unordered_set<size_t> stored_row_ids,
                                 ValueDictionary value_dictionary,
                                 int next_value_id, size_t next_record_id,
                                 CompressedRecords compressed_records);

    size_t GetNextRecordId() const;

    static std::unique_ptr<DynamicRelationData> CreateFrom(config::InputTable const& input_table);

    void InsertBatch(config::InputTable& insert_statements_table);

    void DeleteBatch(std::unordered_set<size_t> const& delete_statement_indices);

    void DeleteRecordsFromUpdateBatch(config::InputTable& update_statements_table);

    void InsertRecordsFromUpdateBatch(config::InputTable& update_statements_table);

    [[nodiscard]] bool IsRowIndexValid(size_t row_id) const;

    bool Empty() const;

    CompressedRecords const& GetCompressedRecords() const;

    CompressedRecordsPtr GetCompressedRecordsPtr() const;
};

}  // namespace algos::dynfd
