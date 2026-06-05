#pragma once

#include <cmath>
#include <memory>
#include <vector>

#include "core/config/tabular_data/input_table_type.h"
#include "encoded_column_data.h"
#include "relation_data.h"
#include "value_dictionary.h"

namespace model {
using ValueDictionaryType = std::shared_ptr<ValueDictionary>;

class ColumnEncodedRelationData final : public AbstractRelationData<EncodedColumnData> {
public:
    explicit ColumnEncodedRelationData(std::unique_ptr<RelationalSchema> schema,
                                       std::vector<ColumnType> column_data) noexcept
        : AbstractRelationData(std::move(schema), std::move(column_data)) {}

    static constexpr int kNullValueId = 0;

    [[nodiscard]] size_t GetNumRows() const final {
        if (column_data_.empty()) {
            return 0;
        }
        return column_data_[0].GetNumRows();
    }

    static std::unique_ptr<ColumnEncodedRelationData> CreateFrom(
            config::InputTable& data_stream, TableIndex table_id,
            ValueDictionaryType value_dictionary);
};
}  // namespace model
