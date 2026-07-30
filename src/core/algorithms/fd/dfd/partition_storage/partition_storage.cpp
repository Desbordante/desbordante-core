#include "core/algorithms/fd/dfd/partition_storage/partition_storage.h"

#include <boost/format.hpp>
#include <boost/optional.hpp>

#include "core/model/table/vertical_map.h"
#include "core/util/logger.h"

namespace {
class PositionListIndexRank {
public:
    boost::dynamic_bitset<> const* vertical_;
    std::shared_ptr<model::PositionListIndex const> pli_;
    int added_arity_;

    PositionListIndexRank(boost::dynamic_bitset<> const* vertical,
                          std::shared_ptr<model::PositionListIndex const> pli, int initial_arity)
        : vertical_(vertical), pli_(pli), added_arity_(initial_arity) {}
};
}  // namespace

model::PositionListIndex const* PartitionStorage::Get(Vertical const& vertical) {
    return index_->Get(vertical.GetColumnIndicesRef()).get();
}

PartitionStorage::PartitionStorage(ColumnLayoutRelationData* relation_data)
    : relation_data_(relation_data),
      index_(std::make_unique<model::BlockingVerticalMap<model::PositionListIndex const>>(
              relation_data->GetSchema())) {
    for (auto& column_ptr : relation_data->GetSchema()->GetColumns()) {
        index_->Put(static_cast<Vertical>(*column_ptr).GetColumnIndicesRef(),
                    relation_data->GetColumnData(column_ptr->GetIndex()).GetPliOwnership());
    }
}

PartitionStorage::~PartitionStorage() {}

// obtains or calculates a PositionListIndex using cache
std::variant<model::PositionListIndex const*, std::unique_ptr<model::PositionListIndex const>>
PartitionStorage::GetOrCreateFor(Vertical const& vertical) {
    std::scoped_lock lock(getting_pli_mutex_);
    LOG_DEBUG("PLI for {} requested: ", vertical.ToString());

    // is PLI already cached?
    model::PositionListIndex const* pli = Get(vertical);
    if (pli != nullptr) {
        LOG_DEBUG("Served from PLI cache.");
        // addToUsageCounter
        return pli;
    }
    // look for cached PLIs to construct the requested one
    auto subset_entries = index_->GetSubsetEntries(vertical.GetColumnIndicesRef());
    boost::optional<PositionListIndexRank> smallest_pli_rank;
    std::vector<PositionListIndexRank> ranks;
    ranks.reserve(subset_entries.size());
    for (auto& [sub_vertical, sub_pli_ptr] : subset_entries) {
        PositionListIndexRank pli_rank(&sub_vertical, sub_pli_ptr, sub_vertical.count());
        ranks.push_back(pli_rank);
        if (!smallest_pli_rank || smallest_pli_rank->pli_->GetSize() > pli_rank.pli_->GetSize() ||
            (smallest_pli_rank->pli_->GetSize() == pli_rank.pli_->GetSize() &&
             smallest_pli_rank->added_arity_ < pli_rank.added_arity_)) {
            smallest_pli_rank = pli_rank;
        }
    }
    assert(smallest_pli_rank);  // check if smallest_pli_rank is initialized

    std::vector<PositionListIndexRank> operands;
    boost::dynamic_bitset<> cover(relation_data_->GetNumColumns());
    boost::dynamic_bitset<> cover_tester(relation_data_->GetNumColumns());
    if (smallest_pli_rank) {
        operands.push_back(*smallest_pli_rank);
        cover |= *smallest_pli_rank->vertical_;

        while (cover.count() < vertical.GetArity() && !ranks.empty()) {
            boost::optional<PositionListIndexRank> best_rank;
            // erase ranks with low added_arity_
            ranks.erase(std::remove_if(ranks.begin(), ranks.end(),
                                       [&cover_tester, &cover](auto& rank) {
                                           cover_tester.reset();
                                           cover_tester |= *rank.vertical_;
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
                cover |= *best_rank->vertical_;
            }
        }
    }

    std::vector<std::unique_ptr<Vertical>> vertical_columns;

    for (auto& column : vertical.GetColumns()) {
        if (!cover[column->GetIndex()]) {
            vertical_columns.push_back(std::make_unique<Vertical>(static_cast<Vertical>(*column)));
            auto column_pli = index_->Get((**vertical_columns.rbegin()).GetColumnIndicesRef());
            operands.emplace_back(&(**vertical_columns.rbegin()).GetColumnIndicesRef(), column_pli,
                                  1);
        }
    }
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
        auto intersection_pli = base_pli_rank.pli_->ProbeAll(
                vertical.Without(
                        relation_data_->GetSchema()->GetVertical(*base_pli_rank.vertical_)),
                *relation_data_);
        variant_intersection_pli = CachingProcess(vertical, std::move(intersection_pli));
    } else {
        Vertical current_vertical =
                relation_data_->GetSchema()->GetVertical(*operands.begin()->vertical_);
        variant_intersection_pli = operands.begin()->pli_.get();

        for (size_t i = 1; i < operands.size(); i++) {
            current_vertical = current_vertical.Union(
                    relation_data_->GetSchema()->GetVertical(*operands[i].vertical_));
            variant_intersection_pli =
                    std::holds_alternative<model::PositionListIndex const*>(
                            variant_intersection_pli)
                            ? std::get<model::PositionListIndex const*>(variant_intersection_pli)
                                      ->Intersect(operands[i].pli_.get())
                            : std::get<std::unique_ptr<model::PositionListIndex const>>(
                                      variant_intersection_pli)
                                      ->Intersect(operands[i].pli_.get());
            variant_intersection_pli = CachingProcess(
                    current_vertical,
                    std::move(std::get<std::unique_ptr<model::PositionListIndex const>>(
                            variant_intersection_pli)));
        }
    }

    LOG_DEBUG("Calculated from {} sub-PLIs (saved {} intersections).", operands.size(),
              (vertical.GetArity() - operands.size()));

    return variant_intersection_pli;
}

std::variant<model::PositionListIndex const*, std::unique_ptr<model::PositionListIndex const>>
PartitionStorage::CachingProcess(Vertical const& vertical,
                                 std::unique_ptr<model::PositionListIndex const> pli) {
    auto pli_pointer = pli.get();
    index_->Put(vertical.GetColumnIndicesRef(), std::move(pli));
    return pli_pointer;
}
