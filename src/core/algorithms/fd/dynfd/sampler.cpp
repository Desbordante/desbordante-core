#include "sampler.h"

#include "dynfd_config.h"

namespace algos::dynfd {

Sampler::Traits::ClusterComparator::ClusterComparator(CompressedRecords const* sort_keys,
                                                      size_t comparison_column_1,
                                                      size_t comparison_column_2,
                                                      Traits const& traits) noexcept
    : sort_keys_(sort_keys),
      comparison_column_1_(comparison_column_1),
      comparison_column_2_(comparison_column_2) {
    assert(sort_keys_->front().size() >= 3);
    assert(comparison_column_1_ < traits.relation_->GetNumColumns());
    assert(comparison_column_2_ < traits.relation_->GetNumColumns());
    if (traits.relation_->GetColumnData(comparison_column_1_)
                .GetPositionListIndex()
                .GetClustersNum() < traits.relation_->GetColumnData(comparison_column_2_)
                                            .GetPositionListIndex()
                                            .GetClustersNum()) {
        std::swap(comparison_column_1_, comparison_column_2_);
    }
}

bool Sampler::Traits::ClusterComparator::operator()(size_t o1, size_t o2) noexcept {
    int value1 = (*sort_keys_)[o1][comparison_column_1_];
    int value2 = (*sort_keys_)[o2][comparison_column_1_];
    if ((value1 < 0 && value2 < 0) || value1 == value2) {
        value1 = (*sort_keys_)[o1][comparison_column_2_];
        value2 = (*sort_keys_)[o2][comparison_column_2_];
    }
    return value1 > value2;
}

void Sampler::Traits::Initialize(hy::Sampler<Traits>& sampler) {
    for (size_t bit = 0; bit < relation_->GetNumColumns(); ++bit) {
        auto clusters_iterator =
                relation_->GetColumnData(bit).GetPositionListIndex().GetClustersToCheck(
                        first_insert_batch_id_);
        sampler.plis_->emplace_back(
                std::make_shared<Traits::PLI>(clusters_iterator.begin(), clusters_iterator.end()));
    }
}

bool Sampler::Traits::IsSingletonCluster(TablePos cluster_id) const noexcept {
    return cluster_id < 0;
}

Sampler::Sampler(std::shared_ptr<DynamicRelationData> relation, size_t first_insert_batch_id,
                 boost::asio::thread_pool* pool)
    : sampler_(std::make_unique<std::vector<Traits::PLIPtr>>(), relation->GetCompressedRecordsPtr(),
               DynFDConfig::kSamplingEfficiencyThreshold, pool, relation->GetNumColumns(),
               Traits{first_insert_batch_id, relation}) {}

algos::hy::ColumnCombinationList Sampler::GetAgreeSets(IdPairs const& comparison_suggestions) {
    return sampler_.GetAgreeSets(comparison_suggestions);
}

}  // namespace algos::dynfd
