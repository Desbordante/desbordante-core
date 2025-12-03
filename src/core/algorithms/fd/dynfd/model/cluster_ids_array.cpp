#include "cluster_ids_array.h"

std::vector<int> const& ClusterIdsArray::GetCluster() const {
    return cluster_ids_;
}

bool ClusterIdsArray::operator==(ClusterIdsArray const& other) const {
    return cluster_ids_ == other.cluster_ids_;
}

bool ClusterIdsArray::operator!=(ClusterIdsArray const& other) const {
    return !(*this == other);
}

ClusterIdsArray ClusterIdsArray::BuildClusterIdsArray(boost::dynamic_bitset<> lhs, size_t lhs_size,
                                                      CompressedRecord record) {
    std::vector<int> cluster_ids(lhs_size);
    size_t index = 0;

    for (size_t lhs_attr = lhs.find_first(); lhs_attr != boost::dynamic_bitset<>::npos;
         lhs_attr = lhs.find_next(lhs_attr)) {
        cluster_ids[index++] = record[lhs_attr];
    }

    return ClusterIdsArray(std::move(cluster_ids));
}

std::size_t std::hash<ClusterIdsArray>::operator()(ClusterIdsArray const& key) const {
    return boost::hash_range(key.GetCluster().begin(), key.GetCluster().end());
}
