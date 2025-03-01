#include "dfd.h"

#include <bits/chrono.h>  // for duration_cast
#include <memory>         // for shared_ptr
#include <utility>        // for pair

#include <boost/asio/post.hpp>                     // for post
#include <boost/asio/thread_pool.hpp>              // for thread_pool
#include <boost/type_index/type_index_facade.hpp>  // for operator==
#include <easylogging++.h>                         // for Writer, CINFO

#include "cache_eviction_method.h"                       // for CacheEvictio...
#include "caching_method.h"                              // for CachingMethod
#include "common_option.h"                               // for CommonOption
#include "config/thread_number/option.h"                 // for kThreadNumbe...
#include "fd/dfd/partition_storage/partition_storage.h"  // for PartitionSto...
#include "fd/pli_based_fd_algorithm.h"                   // for PliBasedFDAl...
#include "lattice_traversal/lattice_traversal.h"         // for LatticeTrave...
#include "model/table/position_list_index.h"             // for PositionList...
#include "model/table/relational_schema.h"               // for RelationalSc...
#include "primitive_collection.h"                        // for PrimitiveCol...
#include "table/column.h"                                // for Column
#include "table/column_data.h"                           // for ColumnData
#include "table/column_layout_relation_data.h"           // for ColumnLayout...
#include "table/vertical.h"                              // for Vertical

namespace algos {

DFD::DFD(std::optional<ColumnLayoutRelationDataManager> relation_manager)
    : PliBasedFDAlgorithm({kDefaultPhaseName}, relation_manager) {
    RegisterOptions();
}

void DFD::RegisterOptions() {
    RegisterOption(config::kThreadNumberOpt(&number_of_threads_));
}

void DFD::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::kThreadNumberOpt.GetName()});
}

void DFD::ResetStateFd() {
    unique_columns_.clear();
}

unsigned long long DFD::ExecuteInternal() {
    auto partition_storage = std::make_unique<PartitionStorage>(
            relation_.get(), CachingMethod::kAllCaching, CacheEvictionMethod::kMedainUsage);
    RelationalSchema const* const schema = relation_->GetSchema();

    auto start_time = std::chrono::system_clock::now();

    // search for unique columns
    for (auto const& column : schema->GetColumns()) {
        ColumnData& column_data = relation_->GetColumnData(column->GetIndex());
        model::PositionListIndex const* const column_pli = column_data.GetPositionListIndex();

        if (column_pli->AllValuesAreUnique()) {
            Vertical const lhs = Vertical(*column);
            unique_columns_.push_back(lhs);
            // we do not register an FD at once, because we check for FDs with empty LHS later
        }
    }

    double progress_step = 100.0 / schema->GetNumColumns();
    boost::asio::thread_pool search_space_pool(number_of_threads_);

    for (auto& rhs : schema->GetColumns()) {
        boost::asio::post(search_space_pool, [this, &rhs, schema, progress_step,
                                              &partition_storage]() {
            ColumnData const& rhs_data = relation_->GetColumnData(rhs->GetIndex());
            model::PositionListIndex const* const rhs_pli = rhs_data.GetPositionListIndex();

            /* if all the rows have the same value, then we register FD with empty LHS
             * if we have minimal FD like []->RHS, it is impossible to find smaller FD with
             * this RHS, so we register it and move to the next RHS
             * */
            if (rhs_pli->GetNepAsLong() == relation_->GetNumTuplePairs()) {
                RegisterFd(*(schema->empty_vertical_), *rhs, relation_->GetSharedPtrSchema());
                AddProgress(progress_step);
                return;
            }

            auto search_space = LatticeTraversal(rhs.get(), relation_.get(), unique_columns_,
                                                 partition_storage.get());
            auto const minimal_deps = search_space.FindLHSs();

            for (auto const& minimal_dependency_lhs : minimal_deps) {
                RegisterFd(minimal_dependency_lhs, *rhs, relation_->GetSharedPtrSchema());
            }
            AddProgress(progress_step);
            LOG(INFO) << static_cast<int>(GetProgress().second);
        });
    }

    search_space_pool.join();
    SetProgress(100);

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    long long apriori_millis = elapsed_milliseconds.count();

    LOG(INFO) << "> FD COUNT: " << fd_collection_.Size();
    LOG(INFO) << "> HASH: " << PliBasedFDAlgorithm::Fletcher16();

    return apriori_millis;
}

}  // namespace algos
