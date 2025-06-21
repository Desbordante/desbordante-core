#include "dynamic_position_list_index.h"

#include <ranges>

#include <easylogging++.h>

namespace algos::dynfd {
DynamicPositionListIndex::Cluster::Cluster(std::vector<size_t> unsorted_records) {
    std::ranges::sort(unsorted_records);

    for (std::vector<size_t> sorted_records = std::move(unsorted_records);
         size_t record_id : sorted_records) {
        records_.push_back(record_id);
    }
}

void DynamicPositionListIndex::Cluster::PushBack(size_t const record_id) {
    records_.push_back(record_id);
}

void DynamicPositionListIndex::Cluster::Erase(size_t const record_id) {
    auto iterator = std::find(records_.begin(), records_.end(), record_id);
    if (iterator != records_.end()) {
        records_.erase(iterator);
    }
}

size_t DynamicPositionListIndex::Cluster::Back() const {
    assert(!Empty());
    return records_.back();
}

bool DynamicPositionListIndex::Cluster::Empty() const {
    return records_.empty();
}

size_t DynamicPositionListIndex::Cluster::Size() const {
    return records_.size();
}

DynamicPositionListIndex::DynamicPositionListIndex(
        std::list<Cluster> clusters,
        std::unordered_map<int, std::list<Cluster>::iterator> inverted_index,
        std::unordered_map<size_t, int> hash_index, int const next_record_id,
        unsigned int const size, model::ColumnIndex columnIndex)
    : clusters_(std::move(clusters)),
      inverted_index_(std::move(inverted_index)),
      hash_index_(std::move(hash_index)),
      next_record_id_(next_record_id),
      size_(size),
      columnIndex_(columnIndex) {}

std::unique_ptr<DynamicPositionListIndex> DynamicPositionListIndex::CreateFor(
        std::vector<int> const &data, model::ColumnIndex columnIndex) {
    std::list<Cluster> clusters;
    std::unordered_map<int, std::list<Cluster>::iterator> inverted_index;
    std::unordered_map<size_t, int> hash_index;
    unsigned int size = data.size();

    int next_record_id = 0;
    for (int const value_id : data) {
        hash_index[next_record_id] = value_id;
        auto inverted_index_it = inverted_index.find(value_id);
        if (inverted_index_it == inverted_index.end()) {
            clusters.emplace_back();
            inverted_index_it = inverted_index.emplace(value_id, std::prev(clusters.end())).first;
        }
        inverted_index_it->second->PushBack(next_record_id++);
    }

    return std::make_unique<DynamicPositionListIndex>(
            std::move(clusters), std::move(inverted_index), std::move(hash_index), next_record_id,
            size, columnIndex);
}

void DynamicPositionListIndex::Erase(size_t const record_id) {
    int const value_id = hash_index_[record_id];
    auto const iterator = inverted_index_.find(value_id);
    iterator->second->Erase(record_id);
    hash_index_.erase(record_id);
    size_--;
    if (iterator->second->Empty()) {
        clusters_.erase(iterator->second);
        inverted_index_.erase(iterator);
    }
}

size_t DynamicPositionListIndex::Insert(int const value_id) {
    int const record_id = next_record_id_++;
    hash_index_[record_id] = value_id;
    if (!inverted_index_.contains(value_id)) {
        clusters_.emplace_back();
        inverted_index_[value_id] = std::prev(clusters_.end());
    }
    inverted_index_[value_id]->PushBack(record_id);
    size_++;
    return record_id;
}

unsigned int DynamicPositionListIndex::GetSize() const {
    return size_;
}

DynamicPositionListIndex::Cluster const &DynamicPositionListIndex::GetCluster(
        int cluster_id) const {
    auto it = clusters_.begin();
    std::advance(it, cluster_id);
    // ReSharper disable once CppDFALocalValueEscapesFunction
    return *it;
}

unsigned int DynamicPositionListIndex::GetClustersNum() const {
    return clusters_.size();
}

int DynamicPositionListIndex::GetRecordValue(size_t record_id) const {
    return hash_index_.at(record_id);
}

std::unique_ptr<DynamicPositionListIndex> DynamicPositionListIndex::FullIntersect(
        DynamicPositionListIndex const &that) const {
    std::unordered_map<int, std::vector<size_t>> partial_index;
    std::list<Cluster> new_clusters;
    std::unordered_map<int, std::list<Cluster>::iterator> new_inverted_index;
    std::unordered_map<size_t, int> new_hash_index;
    unsigned int new_size = 0;

    for (size_t const record_id : hash_index_ | std::views::keys) {
        if (!that.hash_index_.contains(record_id)) {
            LOG(WARNING) << "Record id " << record_id << " not found in that index";
            continue;
        }
        int that_value_id = that.hash_index_.at(record_id);
        partial_index[that_value_id].push_back(record_id);
    }

    for (auto &[value_id, cluster] : partial_index) {
        new_clusters.emplace_back(cluster);
        new_inverted_index[value_id] = std::prev(new_clusters.end());
        new_size += cluster.size();
    }

    return std::make_unique<DynamicPositionListIndex>(
            std::move(new_clusters), std::move(new_inverted_index), std::move(new_hash_index),
            next_record_id_, new_size, columnIndex_);
}

std::string DynamicPositionListIndex::ToString() const {
    std::string res = "[";
    for (auto const &cluster : clusters_) {
        res.push_back('[');
        for (int const v : cluster) {
            res.append(std::to_string(v) + ", ");
        }
        res.erase(res.size() - 2);
        res.push_back(']');
        res += ", ";
    }
    res.erase(res.size() - 2);
    res.push_back(']');
    return res;
}

model::ColumnIndex DynamicPositionListIndex::GetColumnIndex() const {
    return columnIndex_;
}

std::unordered_map<size_t, int> const &DynamicPositionListIndex::GetHashIndex() const {
    return hash_index_;
}

}  // namespace algos::dynfd
