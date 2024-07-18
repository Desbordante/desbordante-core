#pragma once

#include <memory>
#include <string>
#include <tuple>

#include "algorithms/md/hymd/indexes/column_similarity_info.h"
#include "algorithms/md/hymd/indexes/pli_cluster.h"
#include "algorithms/md/hymd/preprocessing/data_info.h"
#include "model/types/numeric_type.h"
#include "model/types/type.h"
#include "util/worker_thread_pool.h"

namespace algos::hymd::preprocessing::similarity_measure {

class SimilarityMeasure {
private:
    std::unique_ptr<model::Type> const arg_type_;
    // Doesn't have to be a double, just any type with total order.
    std::unique_ptr<model::INumericType> const ret_type_;

public:
    SimilarityMeasure(std::unique_ptr<model::Type> arg_type,
                      std::unique_ptr<model::INumericType> ret_type) noexcept
        : arg_type_(std::move(arg_type)), ret_type_(std::move(ret_type)) {}

    virtual ~SimilarityMeasure() = default;

    [[nodiscard]] model::TypeId GetArgTypeId() const noexcept {
        return arg_type_->GetTypeId();
    }

    [[nodiscard]] model::Type const& GetArgType() const noexcept {
        return *arg_type_;
    }

    [[nodiscard]] virtual indexes::SimilarityMeasureOutput MakeIndexes(
            std::shared_ptr<DataInfo const> data_info_left,
            std::shared_ptr<DataInfo const> data_info_right,
            std::vector<indexes::PliCluster> const& clusters_right) const = 0;
};

}  // namespace algos::hymd::preprocessing::similarity_measure
