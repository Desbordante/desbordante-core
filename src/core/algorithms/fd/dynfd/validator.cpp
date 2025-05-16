#include "validator.h"

#include <iostream>
#include <unordered_set>
#include <vector>

#include "fd/hycommon/preprocessor.h"
#include "non_fd_inductor.h"
#include "model/cluster_ids_array.h"

namespace algos::dynfd {

ViolatingRecordPair Validator::FindClusterViolating(const DPLI::Cluster& cluster,
                                                    size_t sortedPlisIndex,
                                                    std::vector<std::shared_ptr<DPLI>>& sorted_plis,
                                                    size_t const rhs) {
    sortedPlisIndex++;
    if (sortedPlisIndex < sorted_plis.size()) {
        auto pli = sorted_plis[sortedPlisIndex];
        auto hash_index = pli->GetHashIndex();
        std::vector<DPLI::Cluster> intersection;
        std::unordered_map<int, int> inverted_index;  // value -> intersection index
        for (size_t record_id : cluster) {
            int value = hash_index[record_id];
            if (auto it = inverted_index.find(value); it == inverted_index.end()) {
                intersection.emplace_back(std::vector({record_id}));
                inverted_index[value] = intersection.size() - 1;
            } else {
                intersection[it->second].PushBack(record_id);
            }
        }
        for (auto const& next_cluster : intersection) {
            auto violation = FindClusterViolating(next_cluster, sortedPlisIndex, sorted_plis, rhs);
            if (violation) {
                return violation;
            }
        }
    } else {
        auto rhs_pli = relation_->GetColumnData(rhs).GetPositionListIndex();
        auto it = cluster.begin();
        int value = rhs_pli->GetRecordValue(*it);
        size_t first_record = *it;
        ++it;
        for (; it != cluster.end(); ++it) {
            if (value != rhs_pli->GetRecordValue(*it)) {
                return {{first_record, *it}};
            }
        }
    }
    return std::nullopt;
}

ViolatingRecordPair Validator::FindEmptyLhsViolation(size_t const rhs) const {
    auto const rhs_pli = relation_->GetColumnData(rhs).GetPositionListIndex();
    if (rhs_pli->GetClustersNum() <= 1) {
        return std::nullopt;
    }

    return {{rhs_pli->GetCluster(0).Back(), rhs_pli->GetCluster(1).Back()}};
}

ViolatingRecordPair Validator::FindNewViolation(RawFD const& nonFd) {
    if (nonFd.lhs_.count() == 0) {
        return FindEmptyLhsViolation(nonFd.rhs_);
    }

    std::vector<std::shared_ptr<DPLI>> sorted_plis = GetSortedPlisForLhs(nonFd.lhs_);

    for (auto const& cluster : *sorted_plis[0]) {
        if (auto violation = FindClusterViolating(cluster, 0, sorted_plis, nonFd.rhs_)) {
            return violation;
        }
    }
    return std::nullopt;
}

std::vector<std::shared_ptr<DPLI>> Validator::GetSortedPlisForLhs(
        boost::dynamic_bitset<> const& lhs) const {
    std::vector<std::shared_ptr<DPLI>> sorted_plis;
    sorted_plis.reserve(lhs.count());
    for (size_t bit = lhs.find_first(); bit != boost::dynamic_bitset<>::npos;
         bit = lhs.find_next(bit)) {
        sorted_plis.push_back(relation_->GetColumnData(bit).GetPositionListIndex());
    }

    std::ranges::sort(sorted_plis, [](auto const& lhs, auto const& rhs) {
        return lhs->GetClustersNum() > rhs->GetClustersNum();
    });

    return sorted_plis;
}

std::shared_ptr<DPLI> Validator::GetFirstPliForLhs(
        boost::dynamic_bitset<> const& lhs) const {
    std::shared_ptr<DPLI> max_clusters_pli;

    for (auto lhs_attr = lhs.find_first(); lhs_attr != boost::dynamic_bitset<>::npos;
         lhs_attr = lhs.find_next(lhs_attr)) {
        auto pli = relation_->GetColumnData(lhs_attr).GetPositionListIndex();
        if (max_clusters_pli.get() == nullptr || max_clusters_pli->GetClustersNum() < pli->GetClustersNum()) {
            max_clusters_pli = pli;
        }
    }

    return max_clusters_pli;
}

ViolatingRecordPair Validator::IsFdInvalidated(RawFD const& fd, size_t first_insert_batch_id) {
    if (fd.lhs_.count() == 0) {
        return FindEmptyLhsViolation(fd.rhs_);
    }

    for (std::vector<std::shared_ptr<DPLI>> sorted_plis = GetSortedPlisForLhs(fd.lhs_);
         auto const& cluster : *sorted_plis[0]) {
        if (cluster.Back() >= first_insert_batch_id) {
            if (auto violation = FindClusterViolating(cluster, 0, sorted_plis, fd.rhs_)) {
                return violation;
            }
        }
    }
    return std::nullopt;
}

bool Validator::NeedsValidation([[maybe_unused]] RawFD const& non_fd) const {
    auto const vertex = negative_cover_tree_->FindNonFdVertex(non_fd.lhs_);
    if (vertex == nullptr) {
        return false;
    }

    return !vertex->IsNonFdViolatingPairHolds(non_fd.rhs_, relation_);
}

boost::dynamic_bitset<> Validator::NeedsValidation(NonFDTreeVertex &vertex) const {
    auto possible_fds = vertex.GetNonFDs();
    for (size_t rhs = possible_fds.find_first(); rhs != boost::dynamic_bitset<>::npos;
         rhs = possible_fds.find_next(rhs)) {
        if (vertex.IsNonFdViolatingPairHolds(rhs, relation_)) {
            possible_fds.reset(rhs);
        }
    }
    return possible_fds;
}

[[nodiscard]] boost::dynamic_bitset<> Validator::Validate(NonFDTreeVertex &vertex,
                                                          boost::dynamic_bitset<> lhs,
                                                          boost::dynamic_bitset<> rhss) {
    auto const lhs_count = lhs.count();
    if (lhs_count == 0) {
        for (size_t rhs = rhss.find_first(); rhs != boost::dynamic_bitset<>::npos;
             rhs = rhss.find_next(rhs)) {
            auto const &violation = FindEmptyLhsViolation(rhs);
            if (violation) {
                vertex.SetViolation(rhs, FindEmptyLhsViolation(rhs));
                rhss.reset(rhs);
            }
        }
        return rhss;
    }

    if (lhs_count == 1) {
        auto const lhs_attr = lhs.find_first();
        auto const pli = relation_->GetColumnData(lhs_attr).GetPositionListIndex();
        for (size_t rhs = rhss.find_first(); rhs != boost::dynamic_bitset<>::npos;
             rhs = rhss.find_next(rhs)) {
            if (!Refines(vertex, *pli, rhs)) {
                rhss.reset(rhs);
            }
        }
        return rhss;
    }

    // std::vector<std::shared_ptr<DPLI>> sorted_plis = GetSortedPlisForLhs(lhs);
    // DPLI const& first_pli = *sorted_plis[0];
    auto first_pli = GetFirstPliForLhs(lhs);
    // DPLI const& first_pli = *relation_->GetColumnData(lhs.find_first()).GetPositionListIndex();
    auto const first_lhs_attr = first_pli->GetColumnIndex();

    lhs.reset(first_lhs_attr);
    return Refines(vertex, *first_pli, lhs, rhss);
}

bool Validator::Refines(NonFDTreeVertex &vertex, algos::dynfd::DPLI const& pli, size_t rhs_attr) const {
    std::vector<CompressedRecord> const& compressed_records = relation_->GetCompressedRecords();

    for (auto const& cluster : pli) {
        auto const rhs_value = compressed_records[*cluster.begin()][rhs_attr];

        for (auto record_id : cluster) {
            auto const value = compressed_records[record_id][rhs_attr];
            if (value != rhs_value) {
                vertex.SetViolation(rhs_attr, std::pair{value, rhs_value});
                return false;
            }
        }
    }

    return true;
}

boost::dynamic_bitset<> Validator::Refines(NonFDTreeVertex &vertex,
                                           algos::dynfd::DPLI const& pli,
                                           boost::dynamic_bitset<> lhs,
                                           boost::dynamic_bitset<> rhss) const {
    auto const lhs_size = lhs.size();
    auto const rhs_size = rhss.size();

    auto const& compressed_records = relation_->GetCompressedRecords();

    std::vector<int> rhs_attr_id_to_ind(relation_->GetNumColumns());
    std::vector<int> rhs_attr_ind_to_id(rhs_size);
    int index = 0;
    for (int rhs = rhss.find_first(); rhs != boost::dynamic_bitset<>::npos;
         rhs = rhss.find_next(rhs)) {
        rhs_attr_id_to_ind[rhs] = index;
        rhs_attr_ind_to_id[index] = rhs;
    }

    for (auto const& cluster : pli) {
        std::unordered_map<ClusterIdsArray, ClusterIdsArrayWithRecord> cluster_ids_map;

        for (auto record_id : cluster) {
            auto cluster_ids_array = ClusterIdsArray::buildClusterIdsArray(
                lhs, lhs_size, compressed_records[record_id]
            );

            auto const cluster_ids_map_it = cluster_ids_map.find(cluster_ids_array);
            if (cluster_ids_map_it != cluster_ids_map.end()) {
                auto const rhs_clusters = cluster_ids_map_it->second;

                for (auto rhs = rhss.find_first(); rhs != boost::dynamic_bitset<>::npos;
                     rhs = rhss.find_next(rhs)) {
                    int rhs_cluster = compressed_records[record_id][rhs];
                    if (rhs_cluster == -1 ||
                        rhs_cluster != rhs_clusters.getCluster()[rhs_attr_id_to_ind[rhs]]) {
                        vertex.SetViolation(rhs, std::pair{record_id, rhs_clusters.getRecordId()});
                        rhss.reset(rhs);
                        if (rhss.empty()) {
                            return rhss;
                        }
                    }
                }
            } else {
                std::vector<int> rhs_clusters(rhs_size);
                for (int rhs_ind = 0; rhs_ind < rhs_size; ++rhs_ind) {
                    rhs_clusters[rhs_ind] = compressed_records[record_id][rhs_attr_ind_to_id[rhs_ind]];
                }

                cluster_ids_map.emplace(
                    std::move(cluster_ids_array),
                    ClusterIdsArrayWithRecord(std::move(rhs_clusters), record_id)
                );
            }
        }
    }

    return rhss;
}

void Validator::ValidateFds(size_t first_insert_batch_id) {
    for (size_t level = 0; level <= relation_->GetNumColumns(); ++level) {
        struct NonFd {
            RawFD rawFd;
            ViolatingRecordPair violation;
        };

        std::vector<NonFd> invalid_fds;
        auto level_fds = positive_cover_tree_->GetLevel(level);
        for (auto& [vertex, lhs] : level_fds) {
            boost::dynamic_bitset<> fds = vertex->GetFDs();
            for (size_t rhs = fds.find_first(); rhs != boost::dynamic_bitset<>::npos;
                 rhs = fds.find_next(rhs)) {
                RawFD fd(lhs, rhs);
                if (auto violation = IsFdInvalidated(fd, first_insert_batch_id)) {
                    invalid_fds.push_back({std::move(fd), std::move(violation)});
                }
            }
        }

        for (auto const& non_fd : invalid_fds) {
            positive_cover_tree_->Remove(non_fd.rawFd.lhs_, non_fd.rawFd.rhs_);
            if (non_fd.rawFd.lhs_.count() > 0) {
                negative_cover_tree_->RemoveGenerals(non_fd.rawFd.lhs_, non_fd.rawFd.rhs_);
            }
            negative_cover_tree_->AddNonFD(non_fd.rawFd.lhs_, non_fd.rawFd.rhs_, non_fd.violation);
            for (size_t new_lhs_attribute = 0; new_lhs_attribute < relation_->GetNumColumns();
                 ++new_lhs_attribute) {
                if (new_lhs_attribute == non_fd.rawFd.rhs_ ||
                    non_fd.rawFd.lhs_.test(new_lhs_attribute)) {
                    continue;
                }

                boost::dynamic_bitset<> new_lhs = non_fd.rawFd.lhs_;
                new_lhs.set(new_lhs_attribute);
                if (!positive_cover_tree_->ContainsFdOrGeneral(new_lhs, non_fd.rawFd.rhs_)) {
                    positive_cover_tree_->AddFD(new_lhs, non_fd.rawFd.rhs_);
                }
            }
        }

        if (static_cast<double>(invalid_fds.size()) > 0.1 * static_cast<double>(level_fds.size())) {
            // TODO: progressive violation search
        }
    }
}

void Validator::ValidateNonFds() {
    NonFDInductor fdFinder(positive_cover_tree_, negative_cover_tree_, shared_from_this());

    for (int level = static_cast<int>(relation_->GetNumColumns()); level >= 0; --level) {
        std::vector<RawFD> valid_fds;
        auto level_non_fds = negative_cover_tree_->GetLevel(level);
        for (auto& [vertex, lhs] : level_non_fds) {
            boost::dynamic_bitset<> possible_fds = NeedsValidation(*vertex);
            if (possible_fds.size() == 0) {
                continue;
            }
            possible_fds = Validate(*vertex, lhs, possible_fds);
            for (size_t rhs = possible_fds.find_first(); rhs != boost::dynamic_bitset<>::npos;
                 rhs = possible_fds.find_next(rhs)) {
                valid_fds.push_back({lhs, rhs});
            }
        }

        for (auto const& fd : valid_fds) {
            negative_cover_tree_->Remove(fd.lhs_, fd.rhs_);
            positive_cover_tree_->RemoveSpecials(fd.lhs_, fd.rhs_);
            positive_cover_tree_->AddFD(fd.lhs_, fd.rhs_);
            for (size_t removed_lhs_attribute = fd.lhs_.find_first();
                 removed_lhs_attribute != boost::dynamic_bitset<>::npos;
                 removed_lhs_attribute = fd.lhs_.find_next(removed_lhs_attribute)) {
                boost::dynamic_bitset<> new_lhs = fd.lhs_;
                new_lhs.reset(removed_lhs_attribute);
                if (!negative_cover_tree_->ContainsNonFdOrSpecial(new_lhs, fd.rhs_)) {
                    negative_cover_tree_->AddNonFD(new_lhs, fd.rhs_, std::nullopt);
                }
            }
        }

        if (static_cast<double>(valid_fds.size()) >
            0.1 * static_cast<double>(level_non_fds.size())) {
            std::cout << "DFS" << std::endl;
            // fdFinder.FindFds(valid_fds);
        }
    }
}

bool Validator::IsNonFdValidated(RawFD const &non_fd) {
    if (NeedsValidation(non_fd)) {
        if (/*auto violation = */FindNewViolation(non_fd)) {
            // vertex->SetViolation(non_fd.rhs_, violation);
            return false;
        } else {
            return true;
        }
    }
    return false;
}

}  // namespace algos::dynfd
