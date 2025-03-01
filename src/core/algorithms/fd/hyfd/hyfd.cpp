#include "hyfd.h"

#include <bits/chrono.h>  // for duration_cast
#include <memory>         // for make_shared, shar...
#include <utility>        // for move
#include <vector>         // for vector

#include <boost/dynamic_bitset/dynamic_bitset.hpp>  // for dynamic_bitset
#include <boost/move/utility_core.hpp>              // for move
#include <boost/type_index/type_index_facade.hpp>   // for operator==
#include <easylogging++.h>                          // for Writer, CTRACE, LOG

#include "algorithms/fd/hycommon/preprocessor.h"  // for Preprocess, Resto...
#include "common_option.h"                        // for CommonOption
#include "config/names.h"                         // for kThreads
#include "config/thread_number/option.h"          // for kThreadNumberOpt
#include "fd/hycommon/types.h"                    // for ClusterId, IdPairs
#include "fd/hyfd/model/fd_tree.h"                // for FDTree
#include "fd/pli_based_fd_algorithm.h"            // for PliBasedFDAlgorithm
#include "fd/raw_fd.h"                            // for RawFD
#include "inductor.h"                             // for Inductor
#include "sampler.h"                              // for Sampler
#include "table/column.h"                         // for Column
#include "table/column_layout_relation_data.h"    // for ColumnLayoutRelat...
#include "table/relational_schema.h"              // for RelationalSchema
#include "table/vertical.h"                       // for Vertical
#include "validator.h"                            // for Validator

namespace algos::hyfd {

HyFD::HyFD(std::optional<ColumnLayoutRelationDataManager> relation_manager)
    : PliBasedFDAlgorithm({}, relation_manager) {
    RegisterOption(config::kThreadNumberOpt(&threads_num_));
}

void HyFD::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::names::kThreads});
}

unsigned long long HyFD::ExecuteInternal() {
    using namespace hy;
    LOG(TRACE) << "Executing";
    auto const start_time = std::chrono::system_clock::now();

    auto [plis, pli_records, og_mapping] = Preprocess(relation_.get());
    auto const plis_shared = std::make_shared<PLIs>(std::move(plis));
    auto const pli_records_shared = std::make_shared<Rows>(std::move(pli_records));

    Sampler sampler(plis_shared, pli_records_shared, threads_num_);

    auto const positive_cover_tree =
            std::make_shared<fd_tree::FDTree>(GetRelation().GetNumColumns());
    Inductor inductor(positive_cover_tree);
    Validator validator(positive_cover_tree, plis_shared, pli_records_shared, threads_num_);

    IdPairs comparison_suggestions;

    while (true) {
        auto non_fds = sampler.GetNonFDs(comparison_suggestions);

        inductor.UpdateFdTree(std::move(non_fds));

        comparison_suggestions = validator.ValidateAndExtendCandidates();

        if (comparison_suggestions.empty()) {
            break;
        }

        LOG(TRACE) << "Cycle done";
    }

    auto fds = positive_cover_tree->FillFDs();
    RegisterFDs(std::move(fds), og_mapping);

    SetProgress(kTotalProgressPercent);

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void HyFD::RegisterFDs(std::vector<RawFD>&& fds, std::vector<hy::ClusterId> const& og_mapping) {
    auto const* const schema = GetRelation().GetSchema();
    for (auto&& [lhs, rhs] : fds) {
        boost::dynamic_bitset<> mapped_lhs =
                hy::RestoreAgreeSet(lhs, og_mapping, schema->GetNumColumns());
        Vertical lhs_v(schema, std::move(mapped_lhs));

        auto const mapped_rhs = og_mapping[rhs];
        Column rhs_c(schema, schema->GetColumn(mapped_rhs)->GetName(), mapped_rhs);

        RegisterFd(std::move(lhs_v), std::move(rhs_c), relation_->GetSharedPtrSchema());
    }
}

}  // namespace algos::hyfd
