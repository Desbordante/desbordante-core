#pragma once

#include <cstddef>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/od/fastod/model/removal_set.h"
#include "core/algorithms/od/fastod/od_ordering.h"
#include "core/algorithms/od/fastod/storage/data_frame.h"
#include "core/algorithms/od/fastod/storage/partition_cache.h"
#include "core/config/indices/type.h"
#include "core/config/tabular_data/input_table_type.h"

namespace algos::od {

class SetBasedAodVerifier final : public Algorithm {
public:
    using Error = double;

private:
    struct OC {
        config::IndicesType context;
        config::IndexType left;
        config::IndexType right;
        Ordering left_ordering = Ordering::_values()[0];
    };

    struct OFD {
        config::IndicesType context;
        config::IndexType right;
    };

    config::InputTable input_table_;
    OC oc_{};
    OFD ofd_{};

    Error error_{0};
    RemovalSet removal_set_{};

    fastod::DataFrame data_{};
    fastod::PartitionCache partition_cache_{};

    void ResetState() final;
    void LoadDataInternal() final;
    void MakeExecuteOptsAvailable() final;
    unsigned long long ExecuteInternal() final;

    void Verify();
    template <od::Ordering Ordering>
    void CalculateRemovalSetForOC();
    void CalculateRemovalSetForOFD();
    template <typename OD>
    void CalculateRemovalSetForOD(OD const& od);

    void RegisterOptions();

public:
    SetBasedAodVerifier();

    [[nodiscard]] bool Holds(Error error = 0.0) const noexcept {
        return error_ <= error;
    }

    [[nodiscard]] Error GetError() const noexcept {
        return error_;
    }

    [[nodiscard]] RemovalSet const& GetRemovalSet() const noexcept {
        return removal_set_;
    }

    [[nodiscard]] size_t GetTupleCount() const noexcept {
        return data_.GetTupleCount();
    }
};

}  // namespace algos::od
