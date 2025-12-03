#pragma once

#include "core/algorithms/fd/hycommon/efficiency_threshold.h"
#include "core/algorithms/fd/hycommon/sampler.h"
#include "core/algorithms/fd/hycommon/types.h"
#include "core/algorithms/fd/hyfd/model/non_fd_list.h"
#include "core/config/thread_number/type.h"
#include "core/model/table/position_list_index.h"

namespace algos::hyfd {

class Sampler {
private:
    struct Traits {
        using PLIsPtr = hy::PLIsPtr;
        using RowsPtr = hy::RowsPtr;
        using IdPairs = hy::IdPairs;
        using PLI = model::PositionListIndex;
        using PLIPtr = PLI*;
        using Cluster = model::PositionListIndex::Cluster;
        using TablePos = hy::TablePos;

        class ClusterComparator {
        private:
            algos::hy::Rows* sort_keys_;
            size_t comparison_column_1_;
            size_t comparison_column_2_;

        public:
            ClusterComparator(algos::hy::Rows* sort_keys, size_t comparison_column_1,
                              size_t comparison_column_2, Traits const&) noexcept
                : sort_keys_(sort_keys),
                  comparison_column_1_(comparison_column_1),
                  comparison_column_2_(comparison_column_2) {
                assert(sort_keys_->front().size() >= 3);
            }

            bool operator()(size_t o1, size_t o2) noexcept;
        };

        void Initialize(hy::Sampler<Traits>& sampler);
        bool IsSingletonCluster(TablePos cluster_id) const noexcept;
    };

    std::unique_ptr<boost::asio::thread_pool> pool_;
    hy::Sampler<Traits> sampler_;

public:
    Sampler(hy::PLIsPtr plis, hy::RowsPtr pli_records, config::ThreadNumType threads_num = 1);
    ~Sampler();
    NonFDList GetNonFDs(hy::IdPairs const& comparison_suggestions);
};

}  // namespace algos::hyfd
