#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "encoded_column_data.h"
#include "idataset_stream.h"
#include "relation_data.h"

namespace model {
class ColumnEncodedRelationData final : public AbstractRelationData<EncodedColumnData> {
public:
    explicit ColumnEncodedRelationData(
            std::unique_ptr<RelationalSchema> schema, std::vector<ColumnType> column_data,
            std::shared_ptr<std::unordered_map<int, std::string>> value_dictionary) noexcept
        : AbstractRelationData(std::move(schema), std::move(column_data)),
          value_dictionary_(value_dictionary) {}

    static constexpr int kNullValueId = -1;

    [[nodiscard]] size_t GetNumRows() const final {
        if (column_data_.empty()) {
            return 0;
        }
        return column_data_[0].GetNumRows();
    }

    static std::unique_ptr<ColumnEncodedRelationData> CreateFrom(model::IDatasetStream& data_stream,
                                                                 TableIndex table_id);

private:
    std::shared_ptr<std::unordered_map<int, std::string>> value_dictionary_;
};
}  // namespace model
