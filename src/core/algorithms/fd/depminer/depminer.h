#pragma once

#include "core/algorithms/algorithm.h"
#include "core/algorithms/fd/bitset_and_index_result_reporter.h"
#include "core/algorithms/fd/lhs_mask_fd_view.h"
#include "core/config/max_lhs/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/stripped_partitions.h"
#include "core/model/table/table_header.h"

namespace algos::fd {

class Depminer : public Algorithm {
private:
    using ColumnCombinations = std::unordered_set<boost::dynamic_bitset<>>;

    config::InputTable input_table_;
    config::MaxLhsType max_lhs_;

    model::TableHeader table_header_;
    model::StrippedPartitions plis_;

    LhsMaskFdView::OwningPointer fd_view_;

    static ColumnCombinations GenFirstLevel(ColumnCombinations const& cmax_set);
    static ColumnCombinations GenNextLevel(ColumnCombinations const& prev_level);
    static bool CheckJoin(boost::dynamic_bitset<> const& p, boost::dynamic_bitset<> const& q);

    void LhsForColumn(model::Index column, BitsetAndIndexResultReporter const& report_fd,
                      ColumnCombinations const& cmax_set);
    std::vector<ColumnCombinations> GenerateCmaxSets(ColumnCombinations const& agree_sets);

    void RegisterOptions();

    void LoadDataInternal() final;

    void MakeExecuteOptsAvailable() final;
    void ResetState() final;

    void ExecuteInternal() final;

public:
    Depminer();

    LhsMaskFdView::OwningPointer GetFds() {
        return fd_view_;
    }
};

}  // namespace algos::fd
