#include "validator.h"

#include <iostream>
#include <unordered_set>
#include <vector>

#include "fd/hycommon/preprocessor.h"

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
        std::unordered_map<int, DPLI::Cluster*> inverted_index;  // value -> cluster
        for (size_t record_id : cluster) {
            int value = hash_index[record_id];
            if (auto it = inverted_index.find(value); it == inverted_index.end()) {
                intersection.emplace_back(std::vector({record_id}));
                inverted_index[value] = &intersection.back();
            } else {
                it->second->PushBack(record_id);
            }
        }
        for (auto const& next_cluster : intersection) {
            return FindClusterViolating(next_cluster, sortedPlisIndex, sorted_plis, rhs);
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
    for (int level = static_cast<int>(relation_->GetNumColumns()); level >= 0; --level) {
        std::vector<RawFD> valid_fds;
        auto level_non_fds = negative_cover_tree_->GetLevel(level);
        for (auto& [vertex, lhs] : level_non_fds) {
            boost::dynamic_bitset<> non_fds = vertex->GetNonFDs();
            for (size_t rhs = non_fds.find_first(); rhs != boost::dynamic_bitset<>::npos;
                 rhs = non_fds.find_next(rhs)) {
                if (RawFD non_fd(lhs, rhs); NeedsValidation(non_fd)) {
                    if (auto violation = FindNewViolation(non_fd)) {
                        vertex->SetViolation(rhs, violation);
                    } else {
                        valid_fds.push_back(non_fd);
                    }
                }
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
            // TODO: depth first search
        }
    }
}

}  // namespace algos::dynfd
