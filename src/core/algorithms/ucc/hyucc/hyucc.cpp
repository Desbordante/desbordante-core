#include "core/algorithms/ucc/hyucc/hyucc.h"

#include "core/algorithms/fd/hycommon/types.h"
#include "core/algorithms/ucc/hyucc/inductor.h"
#include "core/algorithms/ucc/hyucc/preprocessor.h"
#include "core/algorithms/ucc/hyucc/sampler.h"
#include "core/algorithms/ucc/hyucc/validator.h"
#include "core/util/logger.h"

namespace algos {

void HyUCC::LoadDataInternal() {
    schema_ = RelationalSchema::CreateFrom(*input_table_);

    std::tie(plis_, pli_records_, og_mapping_) = hy::Preprocess(*input_table_);
    if (plis_->empty()) {
        throw std::runtime_error("Got an empty dataset: UCC mining is meaningless.");
    }
}

void HyUCC::ExecuteInternal() {
    using namespace hy;
    using namespace hyucc;

    hyucc::Sampler sampler(plis_, pli_records_, threads_num_);

    auto ucc_tree = std::make_unique<UCCTree>(schema_->GetNumColumns());
    Inductor inductor(ucc_tree.get());
    Validator validator(ucc_tree.get(), plis_, pli_records_, threads_num_);

    IdPairs comparison_suggestions;

    while (true) {
        LOG_DEBUG("Sampling...");
        NonUCCList non_uccs = sampler.GetNonUCCs(comparison_suggestions);

        LOG_DEBUG("Inducing...");
        inductor.UpdateUCCTree(std::move(non_uccs));

        LOG_DEBUG("Validating...");
        comparison_suggestions = validator.ValidateAndExtendCandidates();

        if (comparison_suggestions.empty()) {
            break;
        }
    }

    auto uccs = ucc_tree->FillUCCs();
    RegisterUCCs(std::move(uccs), og_mapping_);

    LOG_DEBUG("Mined UCCs:");
    for (model::UCC const& ucc : UCCList()) {
        LOG_DEBUG("{}", ucc.ToString());
    }
}

void HyUCC::RegisterUCCs(std::vector<boost::dynamic_bitset<>>&& uccs,
                         std::vector<hy::ClusterId> const& og_mapping) {
    for (auto&& ucc : uccs) {
        boost::dynamic_bitset<> mapped_ucc =
                hy::RestoreAgreeSet(ucc, og_mapping, schema_->GetNumColumns());
        ucc_collection_.Register(schema_, std::move(mapped_ucc));
    }
}

}  // namespace algos
