#include "algorithms/ucc_verifier/ucc_verifier.h"

#include <chrono>
#include <memory>
#include <stdexcept>

#include "util/config/equal_nulls/option.h"
#include "util/config/indices/option.h"
#include "util/config/indices/validate_index.h"
#include "util/config/names_and_descriptions.h"
#include "util/config/option_using.h"

namespace algos::ucc_verifier {

UCCVerifier::UCCVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({util::config::EqualNullsOpt.GetName()});
}

void UCCVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_schema_cols = [this]() { return relation_->GetSchema()->GetNumColumns(); };

    RegisterOption(util::config::EqualNullsOpt(&is_null_equal_null_));
    RegisterOption(util::config::IndicesOption{kIndices, kDIndices}(&column_indices_, get_schema_cols));
}

void UCCVerifier::MakeExecuteOptsAvailable() {
    using namespace util::config::names;
    MakeOptionsAvailable({kIndices});
}

void UCCVerifier::LoadDataInternal(model::IDatasetStream& data_stream) {
    relation_ = ColumnLayoutRelationData::CreateFrom(data_stream, is_null_equal_null_);
    data_stream.Reset();
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: UCC verifying is meaningless.");
    }
    typed_relation_ =
            model::ColumnLayoutTypedRelationData::CreateFrom(data_stream, is_null_equal_null_);
}

unsigned long long UCCVerifier::ExecuteInternal() {

    auto start_time = std::chrono::system_clock::now();

    stats_calculator_ =
            std::make_unique<StatsCalculator>(relation_, typed_relation_, column_indices_);

    VerifyUCC();
    stats_calculator_->PrintStatistics();



    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);

    return elapsed_milliseconds.count();

}

void UCCVerifier::VerifyUCC() const {
    std::shared_ptr<util::PLI const> pli =
            relation_->GetColumnData(column_indices_[0]).GetPliOwnership();

    for (size_t i = 1; i < column_indices_.size(); ++i) {
        pli = pli->Intersect(relation_->GetColumnData(column_indices_[i]).GetPositionListIndex());
        if (pli->GetNumCluster() == 0)
        {
            return ;
        };
    }

    auto& clusters = pli->GetIndex();
    stats_calculator_->CalculateStatistics(std::move(clusters));
}

}  // namespace algos::ucc_verifier
