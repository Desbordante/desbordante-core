#include "dynamic_relation_data.h"

#include <numeric>
#include <unordered_map>

#include "util/logger.h"

namespace algos::dynfd {

size_t DynamicRelationData::GetNumRows() const {
    if (column_data_.empty()) {
        return 0;
    }
    return column_data_[0].GetNumRows();
}

DynamicRelationData::DynamicRelationData(std::unique_ptr<RelationalSchema> schema,
                                         std::vector<ColumnType> column_data,
                                         std::unordered_set<size_t> stored_row_ids,
                                         ValueDictionary value_dictionary, int next_value_id,
                                         size_t next_record_id,
                                         CompressedRecords compressed_records)
    : AbstractRelationData(std::move(schema), std::move(column_data)),
      stored_row_ids_(std::move(stored_row_ids)),
      value_dictionary_(std::move(value_dictionary)),
      next_value_id_(next_value_id),
      next_record_id_(next_record_id),
      compressed_records_(std::make_shared<CompressedRecords>(std::move(compressed_records))) {}

size_t DynamicRelationData::GetNextRecordId() const {
    return next_record_id_;
}

/**
 * Gets or generates value_id for a field and stores it in compressed records.
 *
 * Strategy:
 * 1. First occurrence:
 *    We assume the field might remain unique. For optimizations in algorithm
 *    we store negative markers instead of the actual ID.
 *    - col_dict[field] = -record_index - 1
 *    - compressed_records = -value_id - 1
 *
 * 2. Second occurrence:
 *    The field is no longer unique. We must fix the previous entry.
 *    - We recover the original record_index from the dictionary.
 *      first_record_index = -col_dict[field] - 1
 *    - We recover the original value_id from the compressed record.
 *      value_id = -compressed_records[first_record_index][col_index] - 1
 *    - We update both the dictionary and the old record to use the positive value_id.
 *
 * 3. Subsequent occurrences:
 *    Standard dictionary lookup.
 *    compressed_records[record_index][col_index] == value_id == col_dict[field]
 */
int DynamicRelationData::GetAndStoreValueId(size_t col_index, std::string const& field,
                                            ValueDictionary& value_dictionary, int& next_value_id,
                                            CompressedRecords& compressed_records) {
    auto& col_dict = value_dictionary[col_index];
    auto location = col_dict.find(field);
    int value_id;

    if (location == col_dict.end()) {
        value_id = next_value_id++;
        col_dict[field] = -static_cast<int>(compressed_records.size());
        compressed_records.back()[col_index] = -value_id - 1;
    } else {
        value_id = location->second;
        if (value_id < 0) {
            int first_record_index = -value_id - 1;
            value_id = -compressed_records[first_record_index][col_index] - 1;
            location->second = value_id;
            compressed_records[first_record_index][col_index] = value_id;
        }
        compressed_records.back()[col_index] = value_id;
    }
    return value_id;
}

void DynamicRelationData::ProcessInputBatch(
        config::InputTable& table, size_t num_columns, size_t col_offset,
        ValueDictionary& value_dictionary, int& next_value_id,
        CompressedRecords& compressed_records,
        std::function<void(size_t col_index, int value_id)> const& on_value,
        std::function<void()> const& on_row_finished) {
    if (table == nullptr) {
        LOG_WARN("Input table is null, skipping batch");
        return;
    }

    size_t const expected_row_size = num_columns + col_offset;

    while (table->HasNextRow()) {
        std::vector<std::string> row = table->GetNextRow();

        if (row.size() != expected_row_size) {
            LOG_WARN("Unexpected number of columns for a row, skipping (expected {}, got {})",
                     expected_row_size, row.size());
            continue;
        }

        compressed_records.emplace_back(num_columns);

        for (size_t index = 0; index < num_columns; ++index) {
            std::string const& field = row[index + col_offset];
            int value_id = GetAndStoreValueId(index, field, value_dictionary, next_value_id,
                                              compressed_records);
            on_value(index, value_id);
        }

        if (on_row_finished) {
            on_row_finished();
        }
    }
    table->Reset();
}

std::unique_ptr<DynamicRelationData> DynamicRelationData::CreateFrom(
        config::InputTable const& input_table) {
    auto schema = std::make_unique<RelationalSchema>(input_table->GetRelationName());
    int next_value_id = 1;
    size_t const num_columns = input_table->GetNumberOfColumns();
    ValueDictionary value_dictionary(num_columns);
    std::vector<std::vector<int>> column_dictionary_encoded_data(num_columns);
    CompressedRecords compressed_records;

    ProcessInputBatch(const_cast<config::InputTable&>(input_table), num_columns, 0,
                      value_dictionary, next_value_id, compressed_records,
                      [&](size_t col_idx, int value_id) {
                          column_dictionary_encoded_data[col_idx].push_back(value_id);
                      });

    std::vector<CompressedColumnData> column_data;
    for (size_t i = 0; i < num_columns; ++i) {
        auto column = Column(schema.get(), input_table->GetColumnName(i), i);
        schema->AppendColumn(std::move(column));
        auto pli = DynamicPositionListIndex::CreateFor(column_dictionary_encoded_data[i], i);
        column_data.emplace_back(schema->GetColumn(i), std::move(pli));
    }

    size_t next_record_id = (!column_data.empty() ? column_data[0].GetNumRows() : 0);
    std::vector<size_t> all_ids(next_record_id);
    std::iota(all_ids.begin(), all_ids.end(), 0);

    return std::make_unique<DynamicRelationData>(std::move(schema), std::move(column_data),
                                                 std::unordered_set(all_ids.begin(), all_ids.end()),
                                                 std::move(value_dictionary), next_value_id,
                                                 next_record_id, std::move(compressed_records));
}

void DynamicRelationData::InsertBatch(config::InputTable& insert_statements_table) {
    ProcessInputBatch(
            insert_statements_table, GetNumColumns(), 0, value_dictionary_, next_value_id_,
            *compressed_records_,
            [this](size_t col_idx, int value_id) {
                size_t const new_record_id =
                        column_data_[col_idx].GetPositionListIndexPtr()->Insert(value_id);
                assert(new_record_id == next_record_id_);
            },
            [this]() { stored_row_ids_.insert(next_record_id_++); });
}

void DynamicRelationData::InsertRecordsFromUpdateBatch(
        config::InputTable& update_statements_table) {
    ProcessInputBatch(
            update_statements_table, GetNumColumns(), 1, value_dictionary_, next_value_id_,
            *compressed_records_,
            [&](size_t col_idx, int value_id) {
                size_t const new_record_id =
                        column_data_[col_idx].GetPositionListIndexPtr()->Insert(value_id);
                assert(new_record_id == next_record_id_);
            },
            [&]() { stored_row_ids_.insert(next_record_id_++); });
}

void DynamicRelationData::DeleteBatch(std::unordered_set<size_t> const& delete_statement_indices) {
    for (size_t row_id : delete_statement_indices) {
        if (!IsRowIndexValid(row_id)) {
            LOG_WARN("Row ID {} is not valid, skipping update", row_id);
            continue;
        }

        for (size_t i = 0; i < GetNumColumns(); ++i) {
            column_data_[i].GetPositionListIndexPtr()->Erase(row_id);
        }
        stored_row_ids_.erase(row_id);
    }
}

void DynamicRelationData::DeleteRecordsFromUpdateBatch(
        config::InputTable& update_statements_table) {
    if (update_statements_table == nullptr) {
        LOG_WARN("Update statements table is null, skipping update batch");
        return;
    }

    while (update_statements_table->HasNextRow()) {
        std::vector<std::string> row = update_statements_table->GetNextRow();

        if (row.size() != GetNumColumns() + 1) {
            LOG_WARN("Unexpected number of columns for a row, skipping (expected {}, got {})",
                     GetNumColumns() + 1, row.size());
            continue;
        }

        size_t row_id = std::stoull(row.front());
        if (!IsRowIndexValid(row_id)) {
            LOG_WARN("Row ID {} is not valid, skipping update", row_id);
            continue;
        }

        for (size_t i = 0; i < GetNumColumns(); ++i) {
            column_data_[i].GetPositionListIndexPtr()->Erase(row_id);
        }
    }

    update_statements_table->Reset();
}

bool DynamicRelationData::IsRowIndexValid(size_t const row_id) const {
    return stored_row_ids_.contains(row_id);
}

bool DynamicRelationData::Empty() const {
    return stored_row_ids_.empty();
}

CompressedRecords const& DynamicRelationData::GetCompressedRecords() const {
    return *compressed_records_;
}

CompressedRecordsPtr DynamicRelationData::GetCompressedRecordsPtr() const {
    return compressed_records_;
}

}  // namespace algos::dynfd
