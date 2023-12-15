#include "ucc_verifier.h"

#include <chrono>
#include <numeric>
#include <stdexcept>

#include "config/equal_nulls/option.h"
#include "config/indices/option.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"

namespace algos {

UCCVerifier::UCCVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::TableOpt.GetName(), config::EqualNullsOpt.GetName()});
}

void UCCVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    auto get_schema_cols = [this]() { return relation_->GetSchema()->GetNumColumns(); };
    auto calculate_default = [get_schema_cols]() {
        config::IndicesType indices(get_schema_cols());
        std::iota(indices.begin(), indices.end(), 0);
        return indices;
    };
    RegisterOption(config::TableOpt(&input_table_));
    RegisterOption(config::EqualNullsOpt(&is_null_equal_null_));
    RegisterOption(config::IndicesOption{kUCCIndices, kDUCCIndices, std::move(calculate_default)}(
            &column_indices_, std::move(get_schema_cols)));
}

void UCCVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kUCCIndices});
}

void UCCVerifier::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: UCC verifying is meaningless.");
    }
}

unsigned long long UCCVerifier::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();
    VerifyUCC();
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void UCCVerifier::VerifyUCC() {
    std::shared_ptr<model::PLI const> pli =
            relation_->GetColumnData(column_indices_[0]).GetPliOwnership();
    for (size_t i = 1; i < column_indices_.size(); ++i) {
        pli = pli->Intersect(relation_->GetColumnData(column_indices_[i]).GetPositionListIndex());
        if (pli->GetNumCluster() == 0) {
            return;
        }
    }
    CalculateStatistics(pli->GetIndex());
}

void UCCVerifier::CalculateStatistics(std::deque<model::PLI::Cluster> const& clusters) {
    for (auto const& cluster : clusters) {
        num_rows_violating_ucc_ += cluster.size();
        clusters_violating_ucc_.push_back(cluster);
    }
}

}  // namespace algos
