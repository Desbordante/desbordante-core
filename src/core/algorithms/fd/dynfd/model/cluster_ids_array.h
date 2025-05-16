#pragma once

#include "dynamic_relation_data.h"

using algos::dynfd::CompressedRecord;

class ClusterIdsArray {
public:
    ClusterIdsArray(std::vector<int> cluster_ids) : cluster_ids_(std::move(cluster_ids)) {}

    std::vector<int> const& getCluster() const;

    bool operator==(ClusterIdsArray const& other) const;

    static ClusterIdsArray buildClusterIdsArray(boost::dynamic_bitset<> lhs,
                                                size_t lhs_size,
                                                CompressedRecord record);

private:
    std::vector<int> cluster_ids_;
};

template<>
struct std::hash<ClusterIdsArray> {
    std::size_t operator()(ClusterIdsArray const& key) const;
};

class ClusterIdsArrayWithRecord : public ClusterIdsArray {
public:
    ClusterIdsArrayWithRecord(std::vector<int> cluster_ids, size_t record_id)
        : ClusterIdsArray(cluster_ids), record_id_(record_id) {}

    size_t getRecordId() const {
        return record_id_;
    }

private:
    size_t const record_id_;
};
