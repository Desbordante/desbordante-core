#include "dc/pli_shard.h"

#include <easylogging++.h>

#include "dc/index_provider.h"
#include "dc/utils.h"

namespace model {

Pli::Pli(std::vector<Cluster> raw_clusters, std::vector<size_t> keys,
         std::unordered_map<size_t, size_t> translator)
    : clusters_(std::move(raw_clusters)),
      keys_(std::move(keys)),
      key_to_cluster_id_map_(std::move(translator)) {}

size_t Pli::GetClusterIdByKey(size_t key) const {
    auto it = key_to_cluster_id_map_.find(key);
    if (it != key_to_cluster_id_map_.end()) {
        return it->second;
    }

    throw std::runtime_error("No cluster can be found by key " + std::to_string(key));
}

size_t Pli::GetFirstIndexWhereKeyIsLTE(size_t target, size_t l) const {
    auto it = std::lower_bound(keys_.begin() + l, keys_.end(), target, std::greater<size_t>());
    return std::distance(keys_.begin(), it);
}

std::string Pli::ToString() const {
    std::stringstream ss;

    ss << "\tKeys: [";
    for (size_t i = 0; i < keys_.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << keys_[i];
    }
    ss << "]\n";
    ss << "\tClusters:\n";
    for (auto const& key : keys_) {
        auto id = GetClusterIdByKey(key);

        ss << "\t\tKey " << key << " -> Cluster #" << id << ": [";
        for (size_t j = 0; j < clusters_[id].size(); ++j) {
            if (j > 0) ss << ", ";
            ss << clusters_[id][j];
        }
        ss << "]\n";
    }
    return ss.str();
}

std::string PliShard::ToString() const {
    std::stringstream ss;
    ss << "Range: [" << beg << ", " << end << ")\n";
    for (size_t i = 0; i < plis.size(); ++i) {
        ss << "PLI #" << (i + 1) << "\n";
        ss << plis[i].ToString();
    }
    return ss.str();
}

PliShardBuilder::PliShardBuilder(size_t shard_length) : shard_length_(shard_length) {
    IntIndexProvider::CreateInstance();
    DoubleIndexProvider::CreateInstance();
    StringIndexProvider::CreateInstance();
}

PliShardBuilder::~PliShardBuilder() {
    IntIndexProvider::ClearInstance();
    DoubleIndexProvider::ClearInstance();
    StringIndexProvider::ClearInstance();
}

template <typename T>
static size_t AddValueToHash(TypedColumnData const& column, size_t row) {
    return IndexProvider<T>::GetInstance()->GetIndex(GetValue<T>(column, row));
}

template <typename T>
static void ColumnToHashTyped(std::vector<size_t>& hashed_column, TypedColumnData const& column) {
    for (size_t i = 0; i < column.GetNumRows(); i++) {
        hashed_column[i] = AddValueToHash<T>(column, i);
    }
}

static std::vector<size_t> ColumnToHash(TypedColumnData const& column) {
    std::vector<size_t> hashed_column(column.GetNumRows());

    switch (column.GetTypeId()) {
        case TypeId::kInt:
            ColumnToHashTyped<int64_t>(hashed_column, column);
            break;
        case TypeId::kDouble:
            ColumnToHashTyped<double>(hashed_column, column);
            break;
        case TypeId::kString:
            ColumnToHashTyped<std::string>(hashed_column, column);
            break;
        default:
            LOG(DEBUG) << "Column type " << column.GetType().ToString()
                       << " is not supported for FastADC's PLI sharding";
            return {};
    }

    return hashed_column;
}

/**
 * FastADC hashes the table with sorted hash, that is, the same values are substituited by
 * the same integers, and higher values are replaced by larger integers. So before
 * creating table with values as keys (hashes), we need to store all values in the
 * related three Providers and sort them.
 */
void AddTableToHashAndSortProviders(std::vector<TypedColumnData> const& input) {
    for (size_t col = 0; col < input.size(); col++) {
        auto const& column = input[col];
        size_t rows = column.GetNumRows();

        switch (column.GetTypeId()) {
            case TypeId::kInt:
                for (size_t row = 0; row < rows; row++) AddValueToHash<int64_t>(column, row);
                break;
            case TypeId::kDouble:
                for (size_t row = 0; row < rows; row++) AddValueToHash<double>(column, row);
                break;
            case TypeId::kString:
                for (size_t row = 0; row < rows; row++) AddValueToHash<std::string>(column, row);
                break;
            default:
                continue;
        }
    }

    IntIndexProvider::GetInstance()->Sort();
    DoubleIndexProvider::GetInstance()->Sort();
}

void PliShardBuilder::BuildPliShards(std::vector<TypedColumnData> const& input) {
    size_t cols_num = input.size();
    assert(cols_num != 0);
    std::vector<std::vector<size_t>> hashed_input(cols_num, std::vector<size_t>());

    AddTableToHashAndSortProviders(input);

    for (size_t i = 0; i < cols_num; ++i) hashed_input[i] = ColumnToHash(input[i]);

    size_t row_beg = 0, row_end = input[0].GetNumRows();
    size_t shards_num = (row_end - row_beg - 1) / shard_length_ + 1;

    pli_shards_.reserve(shards_num);

    for (size_t i = 0; i < shards_num; i++) {
        size_t shard_beg = row_beg + i * shard_length_;
        size_t shard_end = std::min(row_end, shard_beg + shard_length_);
        std::vector<Pli> plis;
        plis.reserve(cols_num);

        for (size_t col = 0; col < cols_num; col++) {
            if (!hashed_input[col].empty()) {
                plis.push_back(
                        BuildPli(hashed_input[col], input[col].IsNumeric(), shard_beg, shard_end));
            }
        }

        pli_shards_.emplace_back(plis, shard_beg, shard_end);
    }
}

Pli PliShardBuilder::BuildPli(std::vector<size_t> const& col_values, bool is_num, size_t beg,
                              size_t end) {
    std::unordered_set<size_t> unique_keys(col_values.begin() + beg, col_values.begin() + end);
    std::vector<size_t> keys(unique_keys.begin(), unique_keys.end());

    if (is_num) {
        std::sort(keys.begin(), keys.end(), std::greater<size_t>());
    }

    std::unordered_map<size_t, size_t> key_to_cluster_id;
    for (size_t i = 0; i < keys.size(); ++i) {
        key_to_cluster_id[keys[i]] = i;
    }

    std::vector<Pli::Cluster> clusters(keys.size());
    for (size_t row = beg; row < end; ++row) {
        size_t key = col_values[row];
        size_t cluster_id = key_to_cluster_id[key];
        clusters[cluster_id].push_back(row);
    }

    return Pli(clusters, keys, key_to_cluster_id);
}

}  // namespace model
