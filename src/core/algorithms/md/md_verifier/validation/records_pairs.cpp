#include "algorithms/md/md_verifier/validation/records_pairs.h"

#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/utility/index_range.h"

namespace algos::md {
void ViolatingRecordsSet::InsertClusters(hymd::indexes::PliCluster const& left_cluster,
                                         hymd::indexes::PliCluster const& right_cluster) {
    for (hymd::RecordIdentifier left_record : left_cluster) {
        RecordsSet& records_set = records_pairs_[left_record];
        for (hymd::RecordIdentifier right_record : right_cluster) {
            records_set.emplace(right_record);
        }
    }
}

void ViolatingRecordsSet::DeleteClusters(hymd::indexes::PliCluster const& left_cluster,
                                         hymd::indexes::PliCluster const& right_cluster) {
    for (hymd::RecordIdentifier left_record : left_cluster) {
        auto it = records_pairs_.find(left_record);
        if (it == records_pairs_.end()) {
            continue;
        }
        RecordsSet& valid_right_records = it->second;

        for (hymd::RecordIdentifier right_record : right_cluster) {
            valid_right_records.erase(right_record);
            if (valid_right_records.empty()) {
                records_pairs_.erase(left_record);
                break;
            }
        }
    }
}

void ViolatingRecordsSet::Fill(std::size_t left_size, std::size_t right_size) {
    Clear();
    for (model::Index left_record : hymd::utility::IndexRange(left_size)) {
        for (model::Index right_record : hymd::utility::IndexRange(right_size)) {
            records_pairs_[left_record].insert(right_record);
        }
    }
}

void ViolatingRecordsSet::Clear() {
    records_pairs_.clear();
}

void IntersectionBuilder::AddIntersection(hymd::indexes::PliCluster const& left_cluster,
                                          hymd::indexes::PliCluster const& right_cluster) {
    for (hymd::RecordIdentifier left_record : left_cluster) {
        auto it = original_.find(left_record);
        if (it == original_.end()) {
            continue;
        }

        RecordsSet const& matched_before = it->second;
        RecordsSet matched_intersection;
        for (hymd::RecordIdentifier right_record : right_cluster) {
            if (matched_before.contains(right_record)) {
                matched_intersection.insert(right_record);
            }
        }

        for (model::Index record : matched_intersection) {
            intersection_[left_record].emplace(record);
        }
    }
}
}  // namespace algos::md