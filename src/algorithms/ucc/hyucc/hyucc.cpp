#include "hyucc.h"

#include <chrono>

#include "hycommon/types.h"
#include "inductor.h"
#include "preprocessor.h"
#include "sampler.h"

namespace algos {

void HyUCC::LoadDataInternal(model::IDatasetStream& data_stream) {
    relation_ = ColumnLayoutRelationData::CreateFrom(data_stream, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: UCC mining is meaningless.");
    }
}

unsigned long long HyUCC::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();

    auto [plis, pli_records, og_mapping] = Preprocess(relation_.get());
    auto const plis_shared = std::make_shared<hy::PLIs>(std::move(plis));
    auto const pli_records_shared = std::make_shared<hy::Rows>(std::move(pli_records));

    Sampler sampler(plis_shared, pli_records_shared);

    auto ucc_tree = std::make_unique<UCCTree>(relation_->GetNumColumns());
    Inductor inductor(ucc_tree.get());

    hy::IdPairs comparison_suggestions;

    while (true) {
        auto non_uccs = sampler.GetNonUCCCandidates(comparison_suggestions);

        inductor.UpdateUCCTree(std::move(non_uccs));
    }

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

}  // namespace algos
