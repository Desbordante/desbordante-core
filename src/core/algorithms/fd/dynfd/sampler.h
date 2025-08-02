#pragma once

#include "algorithms/fd/hycommon/sampler.h"
#include "config/thread_number/type.h"
#include "model/dynamic_position_list_index.h"
#include "model/dynamic_relation_data.h"

namespace algos::dynfd {

class Sampler {
private:
    struct Traits {
        using PLI = std::vector<DPLI::Cluster>;
        using PLIsPtr = std::unique_ptr<std::vector<std::shared_ptr<PLI>>>;
        using PLIPtr = std::shared_ptr<PLI>;
        using RowsPtr = CompressedRecordsPtr;
        using IdPairs = std::vector<std::pair<int, int>>;
        using Cluster = DPLI::Cluster;
        using TablePos = int;

        class ClusterComparator {
        private:
            CompressedRecords const* sort_keys_;
            size_t comparison_column_1_;
            size_t comparison_column_2_;

        public:
            ClusterComparator(CompressedRecords const* sort_keys, size_t comparison_column_1,
                              size_t comparison_column_2, Traits const& traits) noexcept;

            bool operator()(size_t o1, size_t o2) noexcept;
        };

        Traits(size_t first_insert_batch_id, std::shared_ptr<DynamicRelationData> relation)
            : first_insert_batch_id_(first_insert_batch_id), relation_(std::move(relation)) {}

        size_t first_insert_batch_id_;
        std::shared_ptr<DynamicRelationData> relation_;

        void Initialize(hy::Sampler<Traits>& sampler);
        bool IsSingletonCluster(TablePos cluster_id) const noexcept;
    };

    hy::Sampler<Traits> sampler_;

public:
    using IdPairs = Traits::IdPairs;

    Sampler(std::shared_ptr<DynamicRelationData> relation, size_t first_insert_batch_id,
            boost::asio::thread_pool* pool);
    algos::hy::ColumnCombinationList GetAgreeSets(IdPairs const& comparison_suggestions);
};

}  // namespace algos::hyfd
