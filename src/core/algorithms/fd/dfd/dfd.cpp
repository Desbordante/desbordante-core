#include "core/algorithms/fd/dfd/dfd.h"

#include <boost/asio.hpp>

#include "core/algorithms/fd/dfd/lattice_traversal/lattice_traversal.h"
#include "core/config/max_lhs/option.h"
#include "core/config/thread_number/option.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/position_list_index.h"
#include "core/model/table/relational_schema.h"
#include "core/util/logger.h"

namespace algos {

DFD::DFD() : PliBasedFDAlgorithm() {
    RegisterOptions();
}

void DFD::RegisterOptions() {
    RegisterOption(config::kThreadNumberOpt(&number_of_threads_));
}

void DFD::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::kThreadNumberOpt.GetName()});
}

void DFD::ResetStateFd() {}

void DFD::ExecuteInternal() {
    auto partition_storage = std::make_unique<PartitionStorage>(relation_.get());
    RelationalSchema const* const schema = relation_->GetSchema();
    std::vector<boost::dynamic_bitset<>> unique_columns;

    // search for unique columns
    for (model::Index column_index = 0; column_index != schema->GetNumColumns(); ++column_index) {
        ColumnData& column_data = relation_->GetColumnData(column_index);
        model::PositionListIndex const* const column_pli = column_data.GetPositionListIndex();

        if (column_pli->AllValuesAreUnique()) {
            unique_columns.push_back(
                    std::move(boost::dynamic_bitset<>(schema->GetNumColumns()).set(column_index)));
            // we do not register an FD at once, because we check for FDs with empty LHS later
        }
    }

    boost::asio::thread_pool search_space_pool(number_of_threads_);

    for (model::Index rhs_index = 0; rhs_index != schema->GetNumColumns(); ++rhs_index) {
        boost::asio::post(search_space_pool, [this, rhs_index, schema, &partition_storage,
                                              &unique_columns]() {
            ColumnData const& rhs_data = relation_->GetColumnData(rhs_index);
            model::PositionListIndex const* const rhs_pli = rhs_data.GetPositionListIndex();

            /* if all the rows have the same value, then we register FD with empty LHS
             * if we have minimal FD like []->RHS, it is impossible to find smaller FD with
             * this RHS, so we register it and move to the next RHS
             * */
            if (rhs_pli->GetNepAsLong() == relation_->GetNumTuplePairs()) {
                RegisterFd(schema->CreateEmptyVertical(), *schema->GetColumn(rhs_index),
                           relation_->GetSharedPtrSchema());
                return;
            }

            auto search_space = LatticeTraversal(rhs_index, relation_.get(), unique_columns,
                                                 partition_storage.get());
            auto const minimal_deps = search_space.FindLHSs();

            for (auto const& minimal_dependency_lhs : minimal_deps) {
                RegisterFd(schema->GetVertical(minimal_dependency_lhs),
                           *schema->GetColumn(rhs_index), relation_->GetSharedPtrSchema());
            }
        });
    }

    search_space_pool.join();

    LOG_INFO("> FD COUNT: {}", fd_collection_.Size());
    LOG_INFO("> HASH: {}", PliBasedFDAlgorithm::Fletcher16());
}

}  // namespace algos
