#pragma once

#include "algorithms/cfd/cfdfinder/types/cluster_maps.h"
#include "model/table/column_data.h"
#include "model/table/idataset_stream.h"
#include "model/table/relation_data.h"
#include "model/table/relational_schema.h"

namespace algos::cfdfinder {

class CFDFinderRelationData final : public RelationData {
private:
    ClusterMaps cluster_maps_;

public:
    CFDFinderRelationData(std::unique_ptr<RelationalSchema> schema,
                          std::vector<ColumnData> column_data, ClusterMaps cluster_maps)
        : RelationData(std::move(schema), std::move(column_data)),
          cluster_maps_(std::move(cluster_maps)) {}

    using RelationData::AbstractRelationData;

    [[nodiscard]] size_t GetNumRows() const final {
        if (column_data_.empty()) {
            return 0;
        }
        return column_data_[0].GetProbingTable().size();
    }

    ClusterMaps const& GetClusterMaps() const {
        return cluster_maps_;
    }

    static std::unique_ptr<CFDFinderRelationData> CreateFrom(model::IDatasetStream& data_stream,
                                                             bool is_null_eq_null);
};
}  // namespace algos::cfdfinder
