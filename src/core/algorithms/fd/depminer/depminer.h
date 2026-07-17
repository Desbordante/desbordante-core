#pragma once

#include <vector>

#include "core/algorithms/fd/lhs_mask_fd_view.h"
#include "core/algorithms/fd/probing_tables_load_data.h"
#include "core/config/max_lhs/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/table_header.h"

namespace algos::fd {

class Depminer : public ProbingTablesLoadData {
    config::InputTable input_table_;
    config::MaxLhsType max_lhs_;

    model::TableHeader table_header_;

    LhsMaskFdView::OwningPointer fd_view_;

    void RegisterOptions();

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
