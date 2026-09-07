#pragma once

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/dfd/partition_storage/partition_storage.h"
#include "core/algorithms/fd/lhs_mask_fd_view.h"
#include "core/algorithms/fd/probing_tables_load_data.h"
#include "core/config/max_lhs/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/config/thread_number/type.h"
#include "core/model/table/table_header.h"

namespace algos::fd {

class DFD : public ProbingTablesLoadData {
private:
    config::ThreadNumType number_of_threads_;
    config::InputTable input_table_;
    config::MaxLhsType max_lhs_;

    LhsMaskFdView::OwningPointer fd_view_;

    void MakeExecuteOptsAvailable() final;
    void RegisterOptions();

    void ResetState() final;
    void ExecuteInternal() final;

public:
    DFD();

    LhsMaskFdView::OwningPointer GetFds() {
        return fd_view_;
    }
};

}  // namespace algos::fd
