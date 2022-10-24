//
// Created by Ilya Vologin
// https://github.com/cupertank
//

#pragma once

#include <cmath>
#include <vector>

#include "column_data.h"
#include "csv_parser.h"
#include "relational_schema.h"
#include "relation_data.h"

class ColumnLayoutRelationData final : public RelationData {
public:
    static constexpr int kNullValueId = -1;

    using RelationData::AbstractRelationData;

    [[nodiscard]] unsigned int GetNumRows() const final {
        if (column_data_.empty()) {
            return 0;
        }
        return column_data_[0].GetProbingTable().size();
    }
    [[nodiscard]] std::vector<int> GetTuple(int tuple_index) const;

    static std::unique_ptr<ColumnLayoutRelationData> CreateFrom(CSVParser& file_input,
                                                                bool is_null_eq_null,
                                                                int max_cols = -1,
                                                                long max_rows = -1);
};

