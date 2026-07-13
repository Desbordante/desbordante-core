#include "core/algorithms/fd/hyfd/hyfd.h"

#include <algorithm>
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
#include "core/config/names.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/thread_number/option.h"
#include "core/util/logger.h"

namespace algos::hyfd {

HyFD::HyFD() : FDAlgorithm() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kThreadNumberOpt(&threads_num_));

    MakeOptionsAvailable({config::names::kTable});
}

void HyFD::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::names::kThreads});
}

void HyFD::LoadDataInternal() {
    schema_ = RelationalSchema::CreateFrom(*input_table_);

    std::tie(plis_, pli_records_, og_mapping_) = hy::Preprocess(*input_table_);
    if (plis_->empty()) {
        throw std::runtime_error("Got an empty dataset: UCC mining is meaningless.");
    }
}

void HyFD::ExecuteInternal() {
    using namespace hy;
    LOG_TRACE("Executing");

    Sampler sampler(plis_, pli_records_, threads_num_);

    auto const positive_cover_tree = std::make_shared<fd_tree::FDTree>(schema_->GetNumColumns());
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

    auto fds = positive_cover_tree->FillFDs();
    RegisterFDs(std::move(fds), og_mapping_);
}

void HyFD::RegisterFDs(std::vector<RawFD>&& fds, std::vector<hy::ClusterId> const& og_mapping) {
    for (auto&& [lhs, rhs] : fds) {
        boost::dynamic_bitset<> mapped_lhs =
                hy::RestoreAgreeSet(lhs, og_mapping, schema_->GetNumColumns());
        Vertical lhs_v(schema_.get(), std::move(mapped_lhs));

        auto const mapped_rhs = og_mapping[rhs];
        Column rhs_c(schema_.get(), schema_->GetColumn(mapped_rhs)->GetName(), mapped_rhs);

        RegisterFd(std::move(lhs_v), std::move(rhs_c), schema_);
    }
}

}  // namespace algos::hyfd
