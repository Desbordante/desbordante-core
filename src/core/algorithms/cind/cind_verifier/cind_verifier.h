#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "algorithms/algorithm.h"
#include "cind/types.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_tables_type.h"
#include "table/tuple_index.h"

namespace algos::cind {

class CINDVerifier final : public Algorithm {
public:
    using Cluster = std::vector<model::TupleIndex>;

    struct ViolatingCluster {
        Cluster basket_rows;
        Cluster violating_rows;
    };

private:
    config::InputTables input_tables_;

    struct RawIND {
        config::IndicesType lhs;
        config::IndicesType rhs;
    } ind_;

    std::vector<std::string> condition_values_;

    double min_validity_{1.0};
    double min_completeness_{1.0};
    CondType condition_type_;

    std::vector<ViolatingCluster> violating_clusters_;
    std::size_t violating_rows_{0};

    std::size_t supporting_baskets_{0};
    std::size_t included_support_{0};
    std::size_t included_baskets_total_{0};

    double real_validity_{-1.0};
    double real_completeness_{0.0};

private:
    void RegisterOptions();
    void ResetState() final;

    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;

    void VerifyCIND();

public:
    CINDVerifier();

    double GetRealValidity() const noexcept { return real_validity_; }
    double GetRealCompleteness() const noexcept { return real_completeness_; }

    bool Holds() const noexcept {
        return (real_validity_ >= min_validity_) && (real_completeness_ >= min_completeness_);
    }

    std::size_t GetViolatingRowsCount() const noexcept { return violating_rows_; }
    std::size_t GetViolatingClustersCount() const noexcept { return violating_clusters_.size(); }

    std::vector<ViolatingCluster> const& GetViolatingClusters() const noexcept {
        return violating_clusters_;
    }

    std::size_t GetSupportingBaskets() const noexcept { return supporting_baskets_; }
    std::size_t GetIncludedSupportingBaskets() const noexcept { return included_support_; }
    std::size_t GetIncludedBasketsTotal() const noexcept { return included_baskets_total_; }

    void MakeExecuteOptsAvailable() final;
};

}  // namespace algos::cind
