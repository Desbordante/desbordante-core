#include "core/algorithms/fd/dfd/dfd.h"

#include <boost/asio.hpp>

#include "core/algorithms/fd/dfd/lattice_traversal/lattice_traversal.h"
#include "core/algorithms/fd/make_lhs_lim_lhs_mask_adder.h"
#include "core/config/max_lhs/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/thread_number/option.h"
#include "core/model/table/position_list_index.h"
#include "core/util/logger.h"

namespace algos::fd {

DFD::DFD() : ProbingTablesLoadData() {
    RegisterOptions();
}

void DFD::RegisterOptions() {
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
    RegisterOption(config::kThreadNumberOpt(&number_of_threads_));
}

void DFD::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kThreadNumberOpt.GetName(), config::kMaxLhsOpt.GetName()});
}

void DFD::ResetState() {
    fd_view_ = nullptr;
}

void DFD::ExecuteInternal() {
    auto partition_storage = std::make_unique<PartitionStorage>(input_table_column_plis_);
    std::size_t const num_columns = input_table_column_plis_.size();
    std::vector<boost::dynamic_bitset<>> unique_columns;

    // search for unique columns
    for (model::Index column_index = 0; column_index != num_columns; ++column_index) {
        model::PositionListIndex const& column_pli = input_table_column_plis_[column_index];

        if (column_pli.AllValuesAreUnique()) {
            unique_columns.push_back(
                    std::move(boost::dynamic_bitset<>(num_columns).set(column_index)));
            // we do not register an FD at once, because we check for FDs with empty LHS later
        }
    }

    boost::asio::thread_pool search_space_pool(number_of_threads_);
    LhsMaskFdView::Storage lhs_masks(num_columns);

    for (model::Index rhs_index = 0; rhs_index != num_columns; ++rhs_index) {
        boost::asio::post(search_space_pool, [this, &lhs_masks, rhs_index, num_columns,
                                              &partition_storage, &unique_columns]() {
            auto report_fd_lhs = MakeLhsLimLhsMaskAdder(lhs_masks[rhs_index], max_lhs_);
            model::PositionListIndex const& rhs_pli = input_table_column_plis_[rhs_index];

            /* if all the rows have the same value, then we register FD with empty LHS
             * if we have minimal FD like []->RHS, it is impossible to find smaller FD with
             * this RHS, so we register it and move to the next RHS
             * */
            if (rhs_pli.IsConstant()) {
                report_fd_lhs(boost::dynamic_bitset<>(num_columns));
                return;
            }

            auto search_space =
                    LatticeTraversal(rhs_index, input_table_column_plis_, unique_columns,
                                     partition_storage.get(), report_fd_lhs);
            search_space.FindLHSs();
        });
    }

    search_space_pool.join();

    fd_view_ = std::make_shared<LhsMaskFdView>(table_header_, std::move(lhs_masks));
}

}  // namespace algos::fd
