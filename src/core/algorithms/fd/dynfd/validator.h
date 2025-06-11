#pragma once

#include <memory>
#include <utility>

#include <boost/asio/thread_pool.hpp>

#include "model/FDTrees/fd_tree.h"
#include "model/dynamic_position_list_index.h"
#include "model/dynamic_relation_data.h"
#include "model/non_fd_tree.h"

namespace algos::dynfd {
class Validator : public std::enable_shared_from_this<Validator> {
    std::shared_ptr<model::FDTree> positive_cover_tree_;
    std::shared_ptr<NonFDTree> negative_cover_tree_;
    std::shared_ptr<DynamicRelationData> relation_;
    std::unique_ptr<boost::asio::thread_pool> pool_;

    [[nodiscard]] ViolatingRecordPair FindEmptyLhsViolation(size_t rhs) const;

    [[nodiscard]] std::vector<std::shared_ptr<DPLI>> GetSortedPlisForLhs(
            boost::dynamic_bitset<> const &lhs) const;

    [[nodiscard]] std::shared_ptr<DPLI> GetFirstPliForLhs(
            boost::dynamic_bitset<> const &lhs) const;

    [[nodiscard]] bool NeedsValidation(RawFD const &non_fd) const;

    [[nodiscard]] bool NeedsValidation(NonFDTreeVertex &vertex, size_t rhs) const;

    [[nodiscard]] boost::dynamic_bitset<> NeedsValidation(NonFDTreeVertex &vertex, 
                                                          boost::dynamic_bitset<> rhss) const;
    
    std::vector<RawFD> ValidateParallel(std::vector<LhsPair> const& non_fds);

    struct NonFd {
        RawFD rawFd;
        ViolatingRecordPair violation;
    };

    std::vector<NonFd> ValidateParallel(std::vector<model::FDTree::LhsPair> const& fds,
                                        size_t first_insert_batch_id);

    using OnValidateResult = std::optional<std::function<void(size_t, ViolatingRecordPair)>>;

    [[nodiscard]] bool Refines(algos::dynfd::DPLI const& pli,
                               size_t rhs_attr,
                               OnValidateResult const& on_invalid = std::nullopt,
                               size_t first_insert_batch_id = 0) const;

    [[nodiscard]] boost::dynamic_bitset<> Refines(algos::dynfd::DPLI const& pli,
                                                  boost::dynamic_bitset<> lhs,
                                                  boost::dynamic_bitset<> rhss,
                                                  OnValidateResult const& on_invalid = std::nullopt,
                                                  size_t first_insert_batch_id = 0) const;

    boost::dynamic_bitset<> Validate(boost::dynamic_bitset<> lhs,
                                     boost::dynamic_bitset<> rhss,
                                     OnValidateResult const& on_invalid = std::nullopt,
                                     size_t first_insert_batch_id = 0);

public:
    Validator(std::shared_ptr<model::FDTree> positive_cover_tree,
              std::shared_ptr<NonFDTree> negative_cover_tree,
              std::shared_ptr<DynamicRelationData> relation) noexcept
        : positive_cover_tree_(std::move(positive_cover_tree)),
          negative_cover_tree_(std::move(negative_cover_tree)),
          relation_(std::move(relation)),
          pool_(std::make_unique<boost::asio::thread_pool>()) {
    }

    void ValidateFds(size_t first_insert_batch_id);

    void ValidateNonFds();

    [[nodiscard]] bool IsNonFdValidated(RawFD non_fd);

};
}  // namespace algos::dynfd
