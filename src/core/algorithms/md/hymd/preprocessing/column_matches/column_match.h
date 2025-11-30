#pragma once

#include <string>

#include "core/algorithms/md/hymd/indexes/column_similarity_info.h"
#include "core/algorithms/md/hymd/indexes/records_info.h"
#include "core/model/table/relational_schema.h"
#include "core/util/worker_thread_pool.h"

namespace algos::hymd::preprocessing::column_matches {

class ColumnMatch {
private:
    bool is_symmetrical_and_eq_is_max_;
    std::string name_;

public:
    ColumnMatch(bool is_symmetrical_and_eq_is_max, std::string name) noexcept
        : is_symmetrical_and_eq_is_max_(is_symmetrical_and_eq_is_max), name_(std::move(name)) {}

    virtual ~ColumnMatch() = default;

    [[nodiscard]] virtual indexes::ColumnPairMeasurements MakeIndexes(
            util::WorkerThreadPool* pool_ptr, indexes::RecordsInfo const& records_info) const = 0;

    virtual void SetColumns(RelationalSchema const& left_schema,
                            RelationalSchema const& right_schema) = 0;

    virtual std::pair<model::Index, model::Index> GetIndices() const noexcept = 0;

    [[nodiscard]] bool IsSymmetricalAndEqIsMax() const noexcept {
        return is_symmetrical_and_eq_is_max_;
    }

    std::string const& GetName() const noexcept {
        return name_;
    }
};

}  // namespace algos::hymd::preprocessing::column_matches
