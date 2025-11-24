#include "algorithms/cfd/cfdfinder/model/cfdfinder_relation_data.h"

#include <deque>
#include <ranges>
#include <string>

#include <easylogging++.h>

namespace {
std::unique_ptr<model::PositionListIndex> FetchPLI(algos::cfdfinder::ClusterMap& cluster_map,
                                                   bool is_null_equal_null) {
    std::deque<std::vector<int>> clusters;
    std::vector<int> null_cluster;
    unsigned int size = 0;
    unsigned int relation_size = 0;
    static std::string const kNullValue = "";

    if (cluster_map.contains(kNullValue)) {
        null_cluster = cluster_map[kNullValue];
    }
    if (!is_null_equal_null) {
        cluster_map.erase(kNullValue);
        relation_size += null_cluster.size();
    }
    for (auto& iter : cluster_map) {
        size += iter.second.size();
        if (iter.second.size() == 1) {
            continue;
        }
        clusters.push_back(iter.second);
    }
    relation_size += size;
    std::ranges::sort(clusters, {}, [](std::vector<int> const& a) { return a[0]; });

    return std::make_unique<model::PositionListIndex>(std::move(clusters), std::move(null_cluster),
                                                      size, 0, 0, relation_size, relation_size, 0);
}
}  // namespace

namespace algos::cfdfinder {

std::unique_ptr<CFDFinderRelationData> CFDFinderRelationData::CreateFrom(
        model::IDatasetStream& data_stream, [[maybe_unused]] bool is_null_eq_null) {
    auto schema = std::make_unique<RelationalSchema>(data_stream.GetRelationName());
    size_t const num_columns = data_stream.GetNumberOfColumns();
    ClusterMaps cluster_maps(num_columns);

    int row_id = 0;
    std::vector<std::string> row;
    while (data_stream.HasNextRow()) {
        row = data_stream.GetNextRow();

        if (row.size() != num_columns) {
            LOG(WARNING) << "Unexpected number of columns for a row, skipping (expected "
                         << num_columns << ", got " << row.size() << ")";
            continue;
        }

        int attribute_id = 0;
        for (auto&& field : row) {
            auto& cluster_map = cluster_maps[attribute_id++];
            if (!cluster_map.contains(field)) {
                cluster_map[std::move(field)] = std::vector<int>{row_id};
            } else {
                cluster_map[field].push_back(row_id);
            }
        }

        ++row_id;
    }

    std::vector<ColumnData> column_data;
    for (size_t i = 0; i < num_columns; ++i) {
        auto column = Column(schema.get(), data_stream.GetColumnName(i), i);
        schema->AppendColumn(std::move(column));
        auto pli = FetchPLI(cluster_maps[i], is_null_eq_null);
        column_data.emplace_back(schema->GetColumn(i), std::move(pli));
    }

    return std::make_unique<CFDFinderRelationData>(std::move(schema), std::move(column_data),
                                                   std::move(cluster_maps));
}
}  // namespace algos::cfdfinder
