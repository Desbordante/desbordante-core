#pragma once

#include "core/model/table/idataset_stream.h"
#include "core/model/table/relation_data.h"
#include "core/model/table/typed_column_data.h"

namespace model {

using TypedRelationData = AbstractRelationData<TypedColumnData>;

class ColumnLayoutTypedRelationData final : public TypedRelationData {
public:
    using TypedRelationData::AbstractRelationData;

    size_t GetNumRows() const final {
        if (column_data_.empty()) {
            return 0;
        } else {
            return column_data_.front().GetNumRows();
        }
    }

    static std::unique_ptr<ColumnLayoutTypedRelationData> CreateFrom(
            model::IDatasetStream& data_stream, bool is_null_eq_null,
            bool treat_mixed_as_string = false);
};

}  // namespace model
