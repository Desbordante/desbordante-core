#include "core/algorithms/fd/hyfd/hyfd.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/hycommon/preprocessor.h"
#include "core/algorithms/fd/hycommon/util/pli_util.h"
#include "core/algorithms/fd/hyfd/inductor.h"
#include "core/algorithms/fd/hyfd/sampler.h"
#include "core/algorithms/fd/hyfd/validator.h"
#include "core/algorithms/fd/make_reordering_table_mask_pair_adder.h"
#include "core/config/max_lhs/option.h"
#include "core/config/names.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/thread_number/option.h"
#include "core/util/logger.h"

namespace algos::fd::hyfd {

HyFD::HyFD() : Algorithm() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
    RegisterOption(config::kThreadNumberOpt(&threads_num_));

    MakeOptionsAvailable({config::names::kTable});
    // Add multi RHS FD storage builder and so on from the doc...
}

void HyFD::ResetState() {
    fd_view_ = nullptr;
}

void HyFD::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::names::kThreads, config::names::kMaximumLhs});
}

void HyFD::LoadDataInternal() {
    std::tie(plis_, pli_records_, og_mapping_) = hy::Preprocess(*input_table_);
    if (plis_->empty()) {
        throw std::runtime_error("Got an empty dataset: UCC mining is meaningless.");
    }
}

unsigned long long HyFD::ExecuteInternal() {
    using namespace hy;
    LOG_TRACE("Executing");
    auto const start_time = std::chrono::system_clock::now();

    Sampler sampler(plis_, pli_records_, threads_num_);

    auto const positive_cover_tree =
            std::make_shared<fd_tree::FDTree>(table_header_.column_names.size());
    Inductor inductor(positive_cover_tree, max_lhs_);
    Validator validator(positive_cover_tree, plis_, pli_records_, threads_num_, max_lhs_);

    IdPairs comparison_suggestions;

    while (true) {
        auto non_fds = sampler.GetNonFDs(comparison_suggestions);

        inductor.UpdateFdTree(std::move(non_fds));

        comparison_suggestions = validator.ValidateAndExtendCandidates();

        if (comparison_suggestions.empty()) {
            break;
        }

        LOG_TRACE("Cycle done");
    }

    TableMaskPairFdView::Storage storage;
    positive_cover_tree->FillFDs(MakeReorderingTableMaskPairAdder(storage, og_mapping_));
    fd_view_ = std::make_shared<TableMaskPairFdView>(table_header_, std::move(storage));

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

}  // namespace algos::fd::hyfd
