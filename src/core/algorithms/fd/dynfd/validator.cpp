#include "validator.h"

#include <iostream>
#include <unordered_set>
#include <vector>
#include <future>

#include <boost/asio/post.hpp>
#include <boost/thread/future.hpp>
#include <boost/bind/bind.hpp>

#include "fd/hycommon/preprocessor.h"
#include "non_fd_inductor.h"
#include "fd_inductor.h"
#include "model/cluster_ids_array.h"
#include "sampler.h"

namespace algos::dynfd {

ViolatingRecordPair Validator::FindEmptyLhsViolation(size_t const rhs) const {
    auto const& rhs_pli = relation_->GetColumnData(rhs).GetPositionListIndex();
    if (rhs_pli.GetClustersNum() <= 1) {
        return std::nullopt;
    }

    return {{rhs_pli.GetCluster(0).Back(), rhs_pli.GetCluster(1).Back()}};
}

std::vector<std::shared_ptr<DPLI>> Validator::GetSortedPlisForLhs(
        boost::dynamic_bitset<> const& lhs) const {
    std::vector<std::shared_ptr<DPLI>> sorted_plis;
    sorted_plis.reserve(lhs.count());
    for (size_t bit = lhs.find_first(); bit != boost::dynamic_bitset<>::npos;
         bit = lhs.find_next(bit)) {
        sorted_plis.push_back(relation_->GetColumnData(bit).GetPositionListIndexPtr());
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
        auto pli = relation_->GetColumnData(lhs_attr).GetPositionListIndexPtr();
        if (max_clusters_pli.get() == nullptr || max_clusters_pli->GetClustersNum() < pli->GetClustersNum()) {
            max_clusters_pli = pli;
        }
    }

    return max_clusters_pli;
}

bool Validator::NeedsValidation([[maybe_unused]] RawFD const& non_fd) const {
    auto const vertex = negative_cover_tree_->FindNonFdVertex(non_fd.lhs_);
    if (vertex == nullptr) {
        return false;
    }

    return !vertex->IsNonFdViolatingPairHolds(non_fd.rhs_, relation_);
}

bool Validator::NeedsValidation(NonFDTreeVertex& vertex, size_t rhs) const {
    return !vertex.IsNonFdViolatingPairHolds(rhs, relation_);
}

boost::dynamic_bitset<> Validator::NeedsValidation(NonFDTreeVertex& vertex,
                                                   boost::dynamic_bitset<> rhss) const {
    for (size_t rhs = rhss.find_first(); rhs != boost::dynamic_bitset<>::npos;
         rhs = rhss.find_next(rhs)) {
        if (vertex.IsNonFdViolatingPairHolds(rhs, relation_)) {
            rhss.reset(rhs);
        }
    }
    return rhss;
}

boost::dynamic_bitset<> Validator::Validate(boost::dynamic_bitset<> lhs,
                                            boost::dynamic_bitset<> rhss,
                                            OnValidateResult const& on_invalid,
                                            size_t first_insert_batch_id) {
    if (rhss.none()) {
        return rhss;
    }

    auto const lhs_count = lhs.count();
    if (lhs_count == 0) {
        for (size_t rhs = rhss.find_first(); rhs != boost::dynamic_bitset<>::npos;
             rhs = rhss.find_next(rhs)) {
            auto const &violation = FindEmptyLhsViolation(rhs);
            if (violation) {
                if (on_invalid) {
                    (*on_invalid)(rhs, FindEmptyLhsViolation(rhs));
                }
                rhss.reset(rhs);
            }
        }
        return rhss;
    }

    if (lhs_count == 1) {
        auto const lhs_attr = lhs.find_first();
        auto const& pli = relation_->GetColumnData(lhs_attr).GetPositionListIndex();
        for (size_t rhs = rhss.find_first(); rhs != boost::dynamic_bitset<>::npos;
             rhs = rhss.find_next(rhs)) {
            if (!Refines(pli, rhs, on_invalid, first_insert_batch_id)) {
                rhss.reset(rhs);
            }
        }
        return rhss;
    }

    auto first_pli = GetFirstPliForLhs(lhs);
    // auto first_pli = relation_->GetColumnData(lhs.find_first()).GetPositionListIndex();
    auto const first_lhs_attr = first_pli->GetColumnIndex();

    lhs.reset(first_lhs_attr);
    return Refines(*first_pli, lhs, rhss, on_invalid, first_insert_batch_id);
}

std::vector<RawFD> Validator::ValidateParallel(std::vector<LhsPair> const& non_fds) {
    std::vector<RawFD> result;
    std::vector<boost::unique_future<boost::dynamic_bitset<>>> futures;
    futures.reserve(non_fds.size());

    for (auto const& [vertex, lhs] : non_fds) {
        boost::packaged_task<boost::dynamic_bitset<>> task(
            [this, vertex, lhs]() {
                OnValidateResult on_invalid = [vertex](size_t rhs, ViolatingRecordPair violation) {
                    vertex->SetViolation(rhs, violation);
                };
                return this->Validate(lhs, NeedsValidation(*vertex, vertex->GetNonFDs()), on_invalid);
            }
        );
        futures.push_back(task.get_future());
        boost::asio::post(*pool_, std::move(task));
    }

    boost::wait_for_all(futures.begin(), futures.end());

    for (std::size_t index = 0; index < non_fds.size(); ++index) {
        auto const& lhs = non_fds[index].second;
        auto const& rhss = futures[index].get();

        for (auto rhs = rhss.find_first(); rhs != boost::dynamic_bitset<>::npos;
             rhs = rhss.find_next(rhs)) {
            result.push_back({lhs, rhs});
        }
    }

    return result;
}

std::vector<Validator::NonFd> Validator::ValidateParallel(std::vector<model::FDTree::LhsPair> const& fds,
                                                          size_t first_insert_batch_id) {
    std::vector<NonFd> result;
    std::mutex result_mutex;
    std::vector<boost::unique_future<void>> futures;
    futures.reserve(fds.size());

    for (auto const& [vertex, lhs] : fds) {
        boost::packaged_task<void> task(
            [this, vertex, lhs, &result_mutex, &result, first_insert_batch_id]() {
                OnValidateResult on_invalid = [lhs, &result_mutex, &result](size_t rhs, ViolatingRecordPair violation) {
                    std::lock_guard lock(result_mutex);
                    result.push_back({{lhs, rhs}, violation});
                };
                Validate(lhs, vertex->GetFDs(), on_invalid, first_insert_batch_id);
            }
        );
        futures.push_back(task.get_future());
        boost::asio::post(*pool_, std::move(task));
    }

    boost::wait_for_all(futures.begin(), futures.end());
    return result;
}

bool Validator::Refines(algos::dynfd::DPLI const& pli,
                        size_t rhs_attr,
                        OnValidateResult const& on_invalid,
                        size_t first_insert_batch_id) const {
    std::vector<CompressedRecord> const& compressed_records = relation_->GetCompressedRecords();

    for (auto const& cluster : pli.GetClustersToCheck(first_insert_batch_id)) {
        auto const rhs_value = compressed_records[*cluster.begin()][rhs_attr];
        if (rhs_value < 0) {
            if (on_invalid) {
                (*on_invalid)(rhs_attr, std::nullopt);
            }
            return false;
        }

        for (auto record_id : cluster) {
            auto const value = compressed_records[record_id][rhs_attr];
            if (value != rhs_value) {
                if (on_invalid) {
                    (*on_invalid)(rhs_attr, std::pair{value, rhs_value});
                }
                return false;
            }
        }
    }

    return true;
}

boost::dynamic_bitset<> Validator::Refines(algos::dynfd::DPLI const& pli,
                                           boost::dynamic_bitset<> lhs,
                                           boost::dynamic_bitset<> rhss,
                                           OnValidateResult const& on_invalid,
                                           size_t first_insert_batch_id) const {
    auto const lhs_size = lhs.count();
    auto const rhs_size = rhss.count();

    auto const& compressed_records = relation_->GetCompressedRecords();

    std::vector<int> rhs_attr_id_to_ind(relation_->GetNumColumns());
    std::vector<int> rhs_attr_ind_to_id(rhs_size);
    int index = 0;
    for (auto rhs = rhss.find_first(); rhs != boost::dynamic_bitset<>::npos;
         rhs = rhss.find_next(rhs)) {
        rhs_attr_id_to_ind[rhs] = index;
        rhs_attr_ind_to_id[index] = rhs;
        index++;
    }

    for (auto const& cluster : pli.GetClustersToCheck(first_insert_batch_id)) {
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
                    if (rhs_cluster < 0 ||
                        rhs_cluster != rhs_clusters.getCluster()[rhs_attr_id_to_ind[rhs]]) {
                        if (on_invalid) {
                            (*on_invalid)(rhs, std::pair{record_id, rhs_clusters.getRecordId()});
                        }
                        rhss.reset(rhs);
                        if (rhss.none()) {
                            return rhss;
                        }
                    }
                }
            } else {
                std::vector<int> rhs_clusters(rhs_size);
                for (size_t rhs_ind = 0; rhs_ind < rhs_size; ++rhs_ind) {
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
    Sampler sampler(relation_, first_insert_batch_id, pool_.get());
    FDInductor inductor(positive_cover_tree_, negative_cover_tree_);

    for (size_t level = 0; level <= relation_->GetNumColumns(); ++level) {
        auto level_fds = positive_cover_tree_->GetLevel(level);
        std::vector<NonFd> invalid_fds = ValidateParallel(level_fds, first_insert_batch_id);

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
            std::cout << "PVS" << std::endl;
            inductor.UpdateCovers(sampler.GetAgreeSets({}));
        }
    }
}

void Validator::ValidateNonFds() {
    NonFDInductor fdFinder(positive_cover_tree_, negative_cover_tree_, shared_from_this());

    for (int level = static_cast<int>(relation_->GetNumColumns()); level >= 0; --level) {

        auto level_non_fds = negative_cover_tree_->GetLevel(level);
        std::vector<RawFD> valid_fds = ValidateParallel(level_non_fds);

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
            fdFinder.FindFds(valid_fds);
        }
    }
}

bool Validator::IsNonFdValidated(RawFD non_fd) {
    boost::dynamic_bitset<> rhss(non_fd.lhs_.size());
    rhss.set(non_fd.rhs_);
    return Validate(non_fd.lhs_, rhss)[non_fd.rhs_];
}

}  // namespace algos::dynfd
