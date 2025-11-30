//
// Created by Ilya Vologin
// https://github.com/cupertank
//

#pragma once

#include <cmath>
#include <vector>

#include "core/model/table/column_data.h"
#include "core/model/table/idataset_stream.h"
#include "core/model/table/relation_data.h"
#include "core/model/table/relational_schema.h"
#include "core/model/table/position_list_index_with_singletons.h"


class ColumnLayoutRelationData final : public RelationData {
public:
    static constexpr int kNullValueId = -1;

    using RelationData::AbstractRelationData;

    [[nodiscard]] size_t GetNumRows() const final {
        if (column_data_.empty()) {
            return 0;
        }
        return column_data_[0].GetProbingTable().size();
    }

    [[nodiscard]] std::vector<int> GetTuple(int tuple_index) const;

    [[nodiscard]] std::shared_ptr<model::PLI const> CalculatePLI(
            std::vector<unsigned int> const& indices) const;

    [[nodiscard]] std::shared_ptr<model::PLIWS const> CalculatePLIWS(
            std::vector<unsigned int> const& indices) const;

    static std::unique_ptr<ColumnLayoutRelationData> CreateFrom(model::IDatasetStream& data_stream,
                                                                bool is_null_eq_null);
};
