#pragma once

#include <cstddef>
#include <list>
#include <memory>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/cfd/cfun/ccfd.h"
#include "core/algorithms/cfd/cfun/quadruple.h"
#include "core/algorithms/cfd/model/cfd_relation_data.h"
#include "core/config/equal_nulls/type.h"
#include "core/config/tabular_data/input_table_type.h"

namespace algos::cfd::cfun {

class CFUN : public Algorithm {
private:
    using AttributeSet = Quadruple::AttributeSet;
    using Level = std::vector<Quadruple>;

    config::InputTable input_table_;
    unsigned int min_support_ = 1;

    std::unique_ptr<CFDRelationData> cfd_relation_;
    std::list<CCFD> cfd_list_;

    Level BuildZeroLevel() const;
    Level BuildFirstLevel() const;

    std::vector<Quadruple::Partition> CalculateMonoPartitions() const;
    std::vector<std::vector<unsigned int>> GetMonoProbingTables(Level const& first_level) const;

    void ComputeClosures(Level& current, Level& last,
                         std::vector<std::vector<unsigned int>> const& mono_probing_tables);
    CCFD DisplayCFD(Quadruple const& X, unsigned int num_col,
                    std::vector<size_t> const& valid_cluster_id);

    void ComputeQuasiClosures(Level& current, Level const& last) const;
    void GenerateNextLevel(Level& last, Level& current) const;

public:
    CFUN();

    std::list<CCFD> const& GetCFDList() const noexcept {
        return cfd_list_;
    }

    void MakeExecuteOptsAvailable() override;
    void RegisterOptions();
    unsigned long long ExecuteInternal() override;
    void LoadDataInternal() override;

    void ResetState() final {}
};

}  // namespace algos::cfd::cfun
