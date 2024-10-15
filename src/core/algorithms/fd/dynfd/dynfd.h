#pragma once
#include <FDTrees/fd_tree.h>
#include <algorithm.h>
#include <fd/fd.h>
#include <fd/fd_algorithm.h>
#include <tabular_data/input_table_type.h>

#include "fd/hycommon/types.h"
#include "model/dynamic_relation_data.h"
#include "model/non_fd_tree.h"
#include "validator.h"

namespace algos::dynfd {
class DynFD final : public FDAlgorithm {
    config::InputTable input_table_;
    config::InputTable insert_statements_table_ = nullptr;
    config::InputTable update_statements_table_ = nullptr;
    std::unordered_set<size_t> delete_statement_indices_;
    std::shared_ptr<DynamicRelationData> relation_ = nullptr;
    std::shared_ptr<model::FDTree> positive_cover_tree_ = nullptr;
    std::shared_ptr<NonFDTree> negative_cover_tree_ = nullptr;
    std::shared_ptr<Validator> validator_ = nullptr;

public:
    DynFD();
    [[nodiscard]] DynamicRelationData const& GetRelation() const;

private:
    void RegisterOptions();
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailableFDInternal() override;
    unsigned long long ExecuteInternal() override;
    void RegisterFDs(std::vector<RawFD>&& fds);
    void ExecuteHyFD();

    void ResetStateFd() override {}
};

}  // namespace algos::dynfd
