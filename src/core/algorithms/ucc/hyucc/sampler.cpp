#include "sampler.h"

#include "algorithms/fd/hycommon/util/pli_util.h"

namespace algos::hyucc {

bool Sampler::Traits::ClusterComparator::operator()(size_t o1, size_t o2) noexcept {
    size_t value1 = (*sort_keys_)[o1][comparison_column_1_];
    size_t value2 = (*sort_keys_)[o2][comparison_column_1_];
    if (value1 == value2) {
        value1 = (*sort_keys_)[o1][comparison_column_2_];
        value2 = (*sort_keys_)[o2][comparison_column_2_];
    }
    return value1 > value2;
}

void Sampler::Traits::Initialize(hy::Sampler<Traits>&) {}

bool Sampler::Traits::IsSingletonCluster(TablePos cluster_id) const noexcept {
    return hy::PLIUtil::IsSingletonCluster(cluster_id);
}

Sampler::Sampler(hy::PLIsPtr plis, hy::RowsPtr pli_records, config::ThreadNumType threads_num)
    : pool_(threads_num > 1 ? std::make_unique<boost::asio::thread_pool>(threads_num) : nullptr),
      sampler_(plis,
               std::move(pli_records),
               hy::kEfficiencyThreshold,
               pool_.get(),
               plis->size(),
               Traits{}) {}

Sampler::~Sampler() {
    if (pool_ != nullptr) {
        pool_->join();
    }
}

NonUCCList Sampler::GetNonUCCs(hy::IdPairs const& comparison_suggestions) {
    return sampler_.GetAgreeSets(comparison_suggestions);
}

}  // namespace algos::hyfd
