#include "core/algorithms/cfd/model/cfd_relation_data.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <random>

#include "core/algorithms/cfd/util/set_util.h"
#include "core/util/logger.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

size_t CFDRelationData::GetNumRows() const {
    return data_rows_.size();
}

std::unique_ptr<CFDRelationData> CFDRelationData::CreateFrom(model::IDatasetStream& data_stream) {
    size_t num_columns = data_stream.GetNumberOfColumns();
    std::vector<CFDColumnData::ItemDictionary> item_dictionaries(num_columns);
    std::vector<std::vector<int>> columns_values(num_columns);

    std::vector<Transaction> data_rows;
    std::vector<ItemInfo> items;

    std::vector<std::string> row;
    int next_value_id = 1;

    while (data_stream.HasNextRow()) {
        row = data_stream.GetNextRow();

        if (row.size() != num_columns) {
            LOG_WARN(
                    "Unexpected number of columns for a row, "
                    "skipping (expected {}, got {})",
                    num_columns, row.size());
            continue;
        }

        std::vector<int> row_data(num_columns);

        for (size_t index = 0; index < num_columns; ++index) {
            auto [it, inserted] = item_dictionaries[index].try_emplace(row[index], next_value_id);

            if (inserted) {
                items.emplace_back(std::move(row[index]), index);
                columns_values[index].push_back(next_value_id++);
            }

            row_data[index] = it->second;
            items[it->second - 1].frequency++;
        }
        data_rows.push_back(std::move(row_data));
    }

    auto schema = RelationalSchema::CreateFrom(data_stream);
    std::vector<CFDColumnData> column_data;
    for (size_t i = 0; i < num_columns; ++i) {
        column_data.emplace_back(schema->GetColumn(i), std::move(columns_values[i]),
                                 std::move(item_dictionaries[i]));
    }
    return std::make_unique<CFDRelationData>(std::move(schema), std::move(column_data),
                                             std::move(data_rows), std::move(items));
}

unsigned CFDRelationData::GetAttrsNumber() const {
    return column_data_.size();
}

unsigned CFDRelationData::GetItemsNumber() const {
    return items_.size();
}

Transaction const& CFDRelationData::GetRow(unsigned row) const {
    return data_rows_.at(row);
}

void CFDRelationData::SetRow(int row_index, Transaction const& row) {
    for (size_t i = 0; i < row.size(); i++) {
        if (data_rows_[row_index][i] != row[i]) {
            items_[data_rows_[row_index][i] - 1].frequency--;
            items_[row[i] - 1].frequency++;
        }
    }
    data_rows_[row_index] = row;
}

std::vector<int> const& CFDRelationData::GetDomainOfItem(int item) const {
    return column_data_.at(items_[item - 1].attribute).GetValues();
}

std::vector<int> const& CFDRelationData::GetDomain(unsigned attr) const {
    return column_data_.at(attr).GetValues();
}

std::string CFDRelationData::GetAttrName(int index) const {
    return GetSchema()->GetColumn(index)->GetName();
}

int CFDRelationData::GetAttr(std::string const& s) const {
    for (int i = 0; static_cast<size_t>(i) < GetNumColumns(); i++) {
        std::string const ai = GetAttrName(i);
        if (ai == s) {
            return i;
        }
    }
    return -1;
}

int CFDRelationData::GetItem(int attr, std::string const& str_value) const {
    auto const& item_dict = column_data_[attr].GetValueDict();
    auto it = item_dict.find(str_value);

    return it != item_dict.end() ? it->second : -1;
}

unsigned CFDRelationData::Frequency(int i) const {
    return items_[i - 1].frequency;
}

int CFDRelationData::GetAttrIndex(int item_index) const {
    return (item_index > 0) ? items_[item_index - 1].attribute : -1 - item_index;
}

std::string CFDRelationData::GetStringFormat(char delim) const {
    std::string file;
    for (size_t ai = 0; ai < GetNumColumns(); ai++) {
        std::string const attr = GetAttrName(static_cast<int>(ai));
        file += attr;
        if (ai < GetNumColumns() - 1) {
            file += delim;
        } else {
            file += '\n';
        }
    }
    for (auto const& row : data_rows_) {
        for (size_t ri = 0; ri < row.size(); ri++) {
            auto const& item = row[ri];
            file += items_[item - 1].value;
            if (ri < row.size() - 1) {
                file += delim;
            } else {
                file += '\n';
            }
        }
    }
    return file;
}

std::string CFDRelationData::GetStringFormat(SimpleTIdList const& subset, char delim) const {
    std::string result;
    for (size_t ai = 0; ai < GetNumColumns(); ai++) {
        auto const& attr = GetAttrName(static_cast<AttributeIndex>(ai));
        result += attr;
        if (ai < GetNumColumns() - 1) {
            result += delim;
        } else {
            result += '\n';
        }
    }
    for (int i : subset) {
        auto& row = data_rows_[i];
        for (size_t ri = 0; ri < row.size(); ri++) {
            auto const& item = row[ri];
            result += items_[item - 1].value;
            if (ri < row.size() - 1) {
                result += delim;
            } else {
                result += '\n';
            }
        }
    }
    return result;
}

std::string const& CFDRelationData::GetValue(int i) const {
    return items_[i - 1].value;
}

// Receives elements from table and returns its attributes sorted vector
std::vector<int> CFDRelationData::GetAttrVector(Itemset const& items) const {
    std::vector<int> attrs;
    attrs.reserve(items.size());
    for (Item i : items) {
        if (i > 0) {
            attrs.push_back(GetAttrIndex(i));
        } else {
            attrs.push_back(-1 - i);
        }
    }
    std::sort(attrs.begin(), attrs.end());
    return attrs;
}

std::vector<int> CFDRelationData::GetAttrVectorItems(Itemset const& items) const {
    std::vector<int> attrs;
    attrs.reserve(items.size());
    for (Item i : items) {
        if (i > 0) {
            attrs.push_back(-1 - GetAttrIndex(i));
        } else {
            attrs.push_back(i);
        }
    }
    std::sort(attrs.begin(), attrs.end());
    return attrs;
}
}  // namespace algos::cfd
