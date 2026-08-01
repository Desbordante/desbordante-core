#include "core/algorithms/fd/pyrocommon/model/pli_cache.h"

#include <boost/optional.hpp>

#include "core/model/index.h"
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

namespace model {

PositionListIndex const* PLICache::Get(boost::dynamic_bitset<> const& vertical) {
    return index_->Get(vertical).get();
}

PLICache::PLICache(ColumnLayoutRelationData* relation_data, CachingMethod caching_method,
                   CacheEvictionMethod eviction_method, double caching_method_value,
                   double min_entropy, double mean_entropy, double median_entropy,
                   double maximum_entropy, double median_gini, double median_inverted_entropy)
    : relation_data_(relation_data),
      // TODO: сделать
      // index_(std::make_unique<VerticalMap<PositionListIndex>>(relation_data->GetSchema())) при
      // одном потоке
      index_(std::make_unique<BlockingVerticalMap<PositionListIndex const>>(
              relation_data->GetSchema())),
      caching_method_(caching_method),
      eviction_method_(eviction_method),
      caching_method_value_(caching_method_value),
      maximum_entropy_(maximum_entropy),
      mean_entropy_(mean_entropy),
      min_entropy_(min_entropy),
      median_entropy_(median_entropy),
      median_gini_(median_gini),
      median_inverted_entropy_(median_inverted_entropy) {
    for (model::Index column_index = 0;
         column_index != relation_data_->GetSchema()->GetNumColumns(); ++column_index) {
        index_->Put(boost::dynamic_bitset<>(relation_data_->GetSchema()->GetNumColumns())
                            .set(column_index),
                    relation_data->GetColumnData(column_index).GetPliOwnership());
    }
}

PLICache::~PLICache() {
    for (model::Index column_index = 0;
         column_index != relation_data_->GetSchema()->GetNumColumns(); ++column_index) {
        index_->Remove(boost::dynamic_bitset<>(relation_data_->GetSchema()->GetNumColumns())
                               .set(column_index));
    }
}

// obtains or calculates a PositionListIndex using cache
std::variant<PositionListIndex const*, std::unique_ptr<PositionListIndex const>>
PLICache::GetOrCreateFor(boost::dynamic_bitset<> const& vertical,
                         ProfilingContext* profiling_context) {
    std::scoped_lock lock(getting_pli_mutex_);

    // is PLI already cached?
    PositionListIndex const* pli = Get(vertical);
    if (pli != nullptr) {
        LOG_DEBUG("Served from PLI cache.");
        return pli;
    }
    // look for cached PLIs to construct the requested one
    auto subset_entries = index_->GetSubsetEntries(vertical);
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

        while (cover.count() < vertical.count() && !ranks.empty()) {
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

    std::vector<boost::dynamic_bitset<>> vertical_columns;

    util::ForEachIndex(vertical, [&](model::Index column_index) {
        if (cover.test(column_index)) return;
        vertical_columns.push_back(
                boost::dynamic_bitset<>(relation_data_->GetNumColumns()).set(column_index));
    });
    for (boost::dynamic_bitset<> const& vertical : vertical_columns) {
        auto column_pli = index_->Get(vertical);
        operands.emplace_back(&vertical, std::move(column_pli), 1);
    }
    // sort operands by ascending order
    std::sort(operands.begin(), operands.end(),
              [](auto& el1, auto& el2) { return el1.pli_->GetSize() < el2.pli_->GetSize(); });
    // TODO: Profiling context stuff

    LOG_DEBUG("Intersecting [UNIMPLEMENTED].");

    if (operands.empty()) {
        throw std::logic_error("Current implementation assumes operands.size() > 0");
    }

    // TODO: тут не очень понятно: CachingProcess может забрать себе PLI, а может и отдать обратно,
    //  поэтому приходится через variant разбирать. Проверить, насколько много платим за обёртку.
    // Intersect and cache
    std::variant<PositionListIndex const*, std::unique_ptr<PositionListIndex const>>
            variant_intersection_pli;
    if (operands.size() >= profiling_context->GetParameters().nary_intersection_size) {
        PositionListIndexRank base_pli_rank = operands[0];
        auto intersection_pli =
                base_pli_rank.pli_->ProbeAll(vertical - *base_pli_rank.vertical_, *relation_data_);
        variant_intersection_pli =
                CachingProcess(vertical, std::move(intersection_pli), profiling_context);
    } else {
        boost::dynamic_bitset<> current_vertical = *operands.begin()->vertical_;
        variant_intersection_pli = operands.begin()->pli_.get();

        for (size_t i = 1; i < operands.size(); i++) {
            current_vertical |= *operands[i].vertical_;
            variant_intersection_pli =
                    std::holds_alternative<PositionListIndex const*>(variant_intersection_pli)
                            ? std::get<PositionListIndex const*>(variant_intersection_pli)
                                      ->Intersect(operands[i].pli_.get())
                            : std::get<std::unique_ptr<PositionListIndex const>>(
                                      variant_intersection_pli)
                                      ->Intersect(operands[i].pli_.get());
            variant_intersection_pli =
                    CachingProcess(current_vertical,
                                   std::move(std::get<std::unique_ptr<PositionListIndex const>>(
                                           variant_intersection_pli)),
                                   profiling_context);
        }
    }

    LOG_DEBUG("Calculated from {} sub-PLIs (saved {} intersections).", operands.size(),
              (vertical.count() - operands.size()));

    return variant_intersection_pli;
}

std::variant<PositionListIndex const*, std::unique_ptr<PositionListIndex const>>
PLICache::CachingProcess(boost::dynamic_bitset<> const& vertical,
                         std::unique_ptr<PositionListIndex const> pli,
                         ProfilingContext* profiling_context) {
    auto pli_pointer = pli.get();
    switch (caching_method_) {
        case CachingMethod::kCoin:
            if (profiling_context->NextDouble() <
                profiling_context->GetParameters().caching_probability) {
                index_->Put(vertical, std::move(pli));
                return pli_pointer;
            } else {
                return pli;
            }
        case CachingMethod::kNoCaching:
            return pli;
        case CachingMethod::kAllCaching:
            index_->Put(vertical, std::move(pli));
            return pli_pointer;
        default:
            throw std::runtime_error(
                    "Only kNoCaching and kAllCaching strategies are currently available");
    }
}

}  // namespace model
