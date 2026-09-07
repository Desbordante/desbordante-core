#include "core/algorithms/fd/dfd/partition_storage/partition_storage.h"

#include <boost/format.hpp>
#include <boost/optional.hpp>

#include "core/model/index.h"
#include "core/model/table/vertical_map.h"
#include "core/util/bitset_utils.h"
#include "core/util/logger.h"

namespace {
class PositionListIndexRank {
public:
    boost::dynamic_bitset<> pli_columns_;
    model::PositionListIndex const* pli_;
    int added_arity_;

    PositionListIndexRank(boost::dynamic_bitset<> pli_columns, model::PositionListIndex const* pli,
                          int initial_arity)
        : pli_columns_(std::move(pli_columns)), pli_(pli), added_arity_(initial_arity) {}
};
}  // namespace

model::PositionListIndex const* PartitionStorage::Get(boost::dynamic_bitset<> const& vertical) {
    return index_->Get(vertical).get();
}

PartitionStorage::PartitionStorage(
        std::vector<model::PositionListIndex> const& input_table_column_plis)
    : input_table_column_plis_(&input_table_column_plis),
      index_(std::make_unique<model::BlockingVerticalMap<model::PositionListIndex const>>(
              input_table_column_plis.size())) {}

PartitionStorage::~PartitionStorage() {}

// obtains or calculates a PositionListIndex using cache
std::variant<model::PositionListIndex const*, std::unique_ptr<model::PositionListIndex const>>
PartitionStorage::GetOrCreateFor(boost::dynamic_bitset<> const& vertical) {
    std::scoped_lock lock(getting_pli_mutex_);

    // is PLI already cached?
    model::PositionListIndex const* pli = Get(vertical);
    if (pli != nullptr) {
        LOG_DEBUG("Served from PLI cache.");
        return pli;
    }
    // look for cached PLIs to construct the requested one
    auto subset_entries = index_->GetSubsetEntries(vertical);
    boost::optional<PositionListIndexRank> smallest_pli_rank;
    std::vector<PositionListIndexRank> ranks;
    ranks.reserve(subset_entries.size() + vertical.count());
    util::ForEachIndex(vertical, [&](model::Index column_index) {
        PositionListIndexRank const& pli_rank = ranks.emplace_back(
                std::move(boost::dynamic_bitset<>(input_table_column_plis_->size())
                                  .set(column_index)),
                &(*input_table_column_plis_)[column_index], 1);
        if (!smallest_pli_rank || smallest_pli_rank->pli_->GetSize() > pli_rank.pli_->GetSize() ||
            (smallest_pli_rank->pli_->GetSize() == pli_rank.pli_->GetSize() &&
             smallest_pli_rank->added_arity_ < pli_rank.added_arity_)) {
            smallest_pli_rank = pli_rank;
        }
    });
    assert(vertical.any());
    for (auto& [sub_vertical, sub_pli_ptr] : subset_entries) {
        PositionListIndexRank const& pli_rank = ranks.emplace_back(
                std::move(sub_vertical), sub_pli_ptr.get(), sub_vertical.count());
        if (smallest_pli_rank->pli_->GetSize() > pli_rank.pli_->GetSize() ||
            (smallest_pli_rank->pli_->GetSize() == pli_rank.pli_->GetSize() &&
             smallest_pli_rank->added_arity_ < pli_rank.added_arity_)) {
            smallest_pli_rank = pli_rank;
        }
    }
    assert(smallest_pli_rank);

    std::vector<PositionListIndexRank> operands;
    boost::dynamic_bitset<> cover = smallest_pli_rank->pli_columns_;
    boost::dynamic_bitset<> cover_tester;
    operands.push_back(*smallest_pli_rank);

    while (cover.count() < vertical.count() && !ranks.empty()) {
        boost::optional<PositionListIndexRank> best_rank;
        // erase ranks with low added_arity_
        ranks.erase(std::remove_if(ranks.begin(), ranks.end(),
                                   [&cover_tester, &cover](auto& rank) {
                                       cover_tester = rank.pli_columns_;
                                       cover_tester -= cover;
                                       rank.added_arity_ = cover_tester.count();
                                       return rank.added_arity_ < 2;
                                   }),
                    ranks.end());

        for (auto& rank : ranks) {
            if (!best_rank || best_rank->added_arity_ < rank.added_arity_ ||
                (best_rank->added_arity_ == rank.added_arity_ &&
                 best_rank->pli_->GetSize() > rank.pli_->GetSize())) {
                best_rank = rank;
            }
        }

        if (best_rank) {
            operands.push_back(*best_rank);
            cover |= best_rank->pli_columns_;
        }
    }

    util::ForEachIndex(vertical, [&](model::Index column_index) {
        if (cover.test(column_index)) return;
        operands.emplace_back(std::move(boost::dynamic_bitset<>(input_table_column_plis_->size())
                                                .set(column_index)),
                              &(*input_table_column_plis_)[column_index], 1);
    });
    // sort operands by ascending order
    std::sort(operands.begin(), operands.end(),
              [](auto& el1, auto& el2) { return el1.pli_->GetSize() < el2.pli_->GetSize(); });

    LOG_DEBUG("Intersecting [UNIMPLEMENTED]");

    if (operands.empty()) {
        throw std::logic_error("Current implementation assumes operands.size() > 0");
    }

    // Intersect and cache
    std::variant<model::PositionListIndex const*, std::unique_ptr<model::PositionListIndex const>>
            variant_intersection_pli;
    if (operands.size() >= 4) {
        PositionListIndexRank base_pli_rank = operands[0];
        auto intersection_pli = base_pli_rank.pli_->ProbeAll(vertical - base_pli_rank.pli_columns_,
                                                             *input_table_column_plis_);
        variant_intersection_pli = CachingProcess(vertical, std::move(intersection_pli));
    } else {
        boost::dynamic_bitset<> current_vertical = operands.begin()->pli_columns_;
        variant_intersection_pli = operands.begin()->pli_;

        for (size_t i = 1; i < operands.size(); i++) {
            current_vertical |= operands[i].pli_columns_;
            variant_intersection_pli =
                    std::holds_alternative<model::PositionListIndex const*>(
                            variant_intersection_pli)
                            ? std::get<model::PositionListIndex const*>(variant_intersection_pli)
                                      ->Intersect(operands[i].pli_)
                            : std::get<std::unique_ptr<model::PositionListIndex const>>(
                                      variant_intersection_pli)
                                      ->Intersect(operands[i].pli_);
            variant_intersection_pli = CachingProcess(
                    current_vertical,
                    std::move(std::get<std::unique_ptr<model::PositionListIndex const>>(
                            variant_intersection_pli)));
        }
    }

    LOG_DEBUG("Calculated from {} sub-PLIs (saved {} intersections).", operands.size(),
              (vertical.count() - operands.size()));

    return variant_intersection_pli;
}

model::PositionListIndex const* PartitionStorage::CachingProcess(
        boost::dynamic_bitset<> const& vertical,
        std::unique_ptr<model::PositionListIndex const> pli) {
    auto pli_pointer = pli.get();
    index_->Put(vertical, std::move(pli));
    return pli_pointer;
}
