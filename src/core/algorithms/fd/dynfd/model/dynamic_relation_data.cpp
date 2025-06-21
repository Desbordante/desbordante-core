#include "dynamic_relation_data.h"

#include <numeric>
#include <unordered_map>

#include <easylogging++.h>

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

std::unique_ptr<DynamicRelationData> DynamicRelationData::CreateFrom(
        config::InputTable const& input_table) {
    auto schema = std::make_unique<RelationalSchema>(input_table->GetRelationName());
    int next_value_id = 1;
    size_t const num_columns = input_table->GetNumberOfColumns();
    ValueDictionary value_dictionary(num_columns);
    std::vector<std::vector<int>> column_dictionary_encoded_data =
            std::vector<std::vector<int>>(num_columns);
    std::vector<std::vector<int>> compressed_records;

    while (input_table->HasNextRow()) {
        std::vector<std::string> row = input_table->GetNextRow();

        if (row.size() != num_columns) {
            LOG(WARNING) << "Unexpected number of columns for a row, skipping (expected "
                         << num_columns << ", got " << row.size() << ")";
            continue;
        }

        compressed_records.emplace_back(num_columns);

        for (size_t index = 0; index < row.size(); ++index) {
            std::string const& field = row[index];

            auto location = value_dictionary[index].find(field);
            int value_id;
            if (location == value_dictionary[index].end()) {
                value_id = next_value_id++;
                value_dictionary[index][field] = -static_cast<int>(compressed_records.size());

                compressed_records.back()[index] = -value_id - 1;
            } else {
                value_id = location->second;

                if (value_id < 0) {
                    int column_ind = -value_id - 1;
                    value_id = -compressed_records[column_ind][index] - 1;
                    location->second = value_id;
                    compressed_records[column_ind][index] = value_id;
                }

                compressed_records.back()[index] = value_id;
            }

            column_dictionary_encoded_data[index].push_back(value_id);
        }
    }

    std::vector<CompressedColumnData> column_data;
    for (size_t i = 0; i < num_columns; ++i) {
        auto column = Column(schema.get(), input_table->GetColumnName(i), i);
        schema->AppendColumn(std::move(column));
        auto pli = DynamicPositionListIndex::CreateFor(column_dictionary_encoded_data[i], i);
        column_data.emplace_back(schema->GetColumn(i), std::move(pli));
    }

    schema->Init();

    size_t next_record_id = (!column_data.empty() ? column_data[0].GetNumRows() : 0);
    std::vector<size_t> all_ids(next_record_id);
    std::iota(all_ids.begin(), all_ids.end(), 0);

    input_table->Reset();

    return std::make_unique<DynamicRelationData>(std::move(schema), std::move(column_data),
                                                 std::unordered_set(all_ids.begin(), all_ids.end()),
                                                 std::move(value_dictionary), next_value_id,
                                                 next_record_id, std::move(compressed_records));
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void DynamicRelationData::InsertBatch(config::InputTable& insert_statements_table) {
    if (insert_statements_table == nullptr) {
        LOG(WARNING) << "Insert statements table is null, skipping insert batch";
        return;
    }

    while (insert_statements_table->HasNextRow()) {
        std::vector<std::string> row = insert_statements_table->GetNextRow();

        if (row.size() != GetNumColumns()) {
            LOG(WARNING) << "Unexpected number of columns for a row, skipping (expected "
                         << GetNumColumns() << ", got " << row.size() << ")";
            continue;
        }

        compressed_records_->emplace_back(row.size());

        for (size_t index = 0; index < row.size(); ++index) {
            std::string const& field = row[index];

            int value_id;
            if (auto location = value_dictionary_[index].find(field);
                location == value_dictionary_[index].end()) {
                value_id = next_value_id_++;
                value_dictionary_[index][field] = -static_cast<int>(compressed_records_->size());

                compressed_records_->back()[index] = -value_id - 1;
            } else {
                value_id = location->second;

                if (value_id < 0) {
                    int column_ind = -value_id - 1;
                    value_id = -(*compressed_records_)[column_ind][index] - 1;
                    location->second = value_id;
                    (*compressed_records_)[column_ind][index] = value_id;
                }

                compressed_records_->back()[index] = value_id;
            }

            size_t const new_record_id =
                    column_data_[index].GetPositionListIndexPtr()->Insert(value_id);
            assert(new_record_id == next_record_id_);
        }

        stored_row_ids_.insert(next_record_id_++);
    }

    insert_statements_table->Reset();
}

void DynamicRelationData::DeleteBatch(std::unordered_set<size_t> const& delete_statement_indices) {
    for (size_t row_id : delete_statement_indices) {
        if (!IsRowIndexValid(row_id)) {
            LOG(WARNING) << "Row ID " << row_id << " is not valid, skipping update";
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
        LOG(WARNING) << "Update statements table is null, skipping update batch";
        return;
    }

    while (update_statements_table->HasNextRow()) {
        std::vector<std::string> row = update_statements_table->GetNextRow();

        if (row.size() != GetNumColumns() + 1) {
            LOG(WARNING) << "Unexpected number of columns for a row, skipping (expected "
                         << GetNumColumns() + 1 << ", got " << row.size() << ")";
            continue;
        }

        size_t row_id = std::stoull(row.front());
        if (!IsRowIndexValid(row_id)) {
            LOG(WARNING) << "Row ID " << row_id << " is not valid, skipping update";
            continue;
        }

        for (size_t i = 0; i < GetNumColumns(); ++i) {
            column_data_[i].GetPositionListIndexPtr()->Erase(row_id);
        }
    }

    update_statements_table->Reset();
}

void DynamicRelationData::InsertRecordsFromUpdateBatch(
        config::InputTable& update_statements_table) {
    if (update_statements_table == nullptr) {
        LOG(WARNING) << "Update statements table is null, skipping update batch";
        return;
    }

    while (update_statements_table->HasNextRow()) {
        std::vector<std::string> row = update_statements_table->GetNextRow();

        if (row.size() != GetNumColumns() + 1) {
            LOG(WARNING) << "Unexpected number of columns for a row, skipping (expected "
                         << GetNumColumns() + 1 << ", got " << row.size() << ")";
            continue;
        }

        compressed_records_->emplace_back(row.size());

        for (size_t index = 0; index < GetNumColumns(); ++index) {
            std::string const& field = row[index + 1];

            int value_id;
            if (auto location = value_dictionary_[index].find(field);
                location == value_dictionary_[index].end()) {
                value_id = next_value_id_++;
                value_dictionary_[index][field] = -static_cast<int>(compressed_records_->size());

                compressed_records_->back()[index] = -value_id - 1;
            } else {
                value_id = location->second;

                if (value_id < 0) {
                    int column_ind = -value_id - 1;
                    value_id = -(*compressed_records_)[column_ind][index] - 1;
                    location->second = value_id;
                    (*compressed_records_)[column_ind][index] = value_id;
                }

                compressed_records_->back()[index] = value_id;
            }

            size_t const new_record_id =
                    column_data_[index].GetPositionListIndexPtr()->Insert(value_id);
            assert(new_record_id == next_record_id_);
        }
        stored_row_ids_.insert(next_record_id_++);
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
