#include "algorithms/md/hymd/hymd.h"

#include <algorithm>
#include <cstddef>
#include <limits>

#include "algorithms/md/hymd/lattice/cardinality/min_picking_level_getter.h"
#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "algorithms/md/hymd/lattice_traverser.h"
#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"
#include "algorithms/md/hymd/record_pair_inferrer.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/utility/md_less.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/thread_number/option.h"
#include "model/index.h"
#include "model/table/column.h"
#include "util/worker_thread_pool.h"

namespace algos::hymd {

using model::Index;

HyMD::HyMD() : MdAlgorithm({}) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kLeftTable, kRightTable});
}

void HyMD::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable(
            {kMinSupport, kPruneNonDisjoint, kColumnMatches, kMaxCardinality, kThreads});
}

void HyMD::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto min_support_default = [this]() {
        if (records_info_->OneTableGiven()) {
            return records_info_->GetLeftCompressor().GetRecords().size() + 1;
        } else {
            return std::size_t(1);
        }
    };
    auto min_support_check = [this](std::size_t const& min_sup_value) {
        if (min_sup_value > records_info_->GetTotalPairsNum())
            throw config::ConfigurationError(
                    "Support is greater than the number of pairs, mining MDs will be meaningless!");
    };

    auto column_matches_default = [this]() {
        std::vector<std::tuple<std::string, std::string, std::shared_ptr<SimilarityMeasureCreator>>>
                column_matches_option;
        if (records_info_->OneTableGiven()) {
            std::size_t const num_columns = left_schema_->GetNumColumns();
            column_matches_option.reserve(num_columns);
            for (Index i = 0; i < num_columns; ++i) {
                std::string const column_name = left_schema_->GetColumn(i)->GetName();
                column_matches_option.emplace_back(
                        column_name, column_name,
                        std::make_shared<preprocessing::similarity_measure::
                                                 LevenshteinSimilarityMeasure::Creator>(0.7, 0));
            }
        } else {
            std::size_t const num_columns_left = left_schema_->GetNumColumns();
            std::size_t const num_columns_right = left_schema_->GetNumColumns();
            column_matches_option.reserve(num_columns_left * num_columns_right);
            for (Index i = 0; i < num_columns_left; ++i) {
                std::string const column_name_left = left_schema_->GetColumn(i)->GetName();
                for (Index j = 0; j < num_columns_right; ++j) {
                    std::string const column_name_right = right_schema_->GetColumn(j)->GetName();
                    column_matches_option.emplace_back(
                            column_name_left, column_name_right,
                            std::make_shared<preprocessing::similarity_measure::
                                                     LevenshteinSimilarityMeasure::Creator>(0.7,
                                                                                            0));
                }
            }
        }
        return column_matches_option;
    };

    auto not_null = [](config::InputTable const& table) {
        if (table == nullptr) throw config::ConfigurationError("Left table may not be null.");
    };
    auto column_matches_check = [this](ColMatchesVector const& col_matches) {
        if (col_matches.empty())
            throw config::ConfigurationError("Mining with empty column matches is meaningless.");
        for (auto const& [left_name, right_name, creator] : col_matches) {
            if (!left_schema_->IsColumnInSchema(left_name))
                throw config::ConfigurationError("Column " + left_name + " is not in left table.");
            if (!right_schema_->IsColumnInSchema(right_name))
                throw config::ConfigurationError("Column " + right_name +
                                                 " is not in right table.");
        }
    };

    RegisterOption(Option{&right_table_, kRightTable, kDRightTable, config::InputTable{nullptr}});
    RegisterOption(Option{&left_table_, kLeftTable, kDLeftTable}
                           .SetValueCheck(not_null)
                           .SetConditionalOpts({{{}, {kRightTable}}}));

    RegisterOption(
            Option{&min_support_, kMinSupport, kDMinSupport, {min_support_default}}.SetValueCheck(
                    min_support_check));
    RegisterOption(Option{&prune_nondisjoint_, kPruneNonDisjoint, kDPruneNonDisjoint, true});
    RegisterOption(Option{
            &column_matches_option_, kColumnMatches, kDColumnMatches, {column_matches_default}}
                           .SetValueCheck(column_matches_check));
    RegisterOption(Option{&max_cardinality_, kMaxCardinality, kDMaxCardinality,
                          std::numeric_limits<std::size_t>::max()});
    RegisterOption(config::kThreadNumberOpt(&threads_));
}

void HyMD::ResetStateMd() {}

void HyMD::LoadDataInternal() {
    left_schema_ = std::make_shared<RelationalSchema>(left_table_->GetRelationName());
    std::size_t const left_table_cols = left_table_->GetNumberOfColumns();
    for (Index i = 0; i < left_table_cols; ++i) {
        left_schema_->AppendColumn(left_table_->GetColumnName(i));
    }
    if (right_table_ == nullptr) {
        right_schema_ = left_schema_;
        records_info_ = indexes::RecordsInfo::CreateFrom(*left_table_);
    } else {
        right_schema_ = std::make_unique<RelationalSchema>(right_table_->GetRelationName());
        std::size_t const right_table_cols = right_table_->GetNumberOfColumns();
        for (Index i = 0; i < right_table_cols; ++i) {
            right_schema_->AppendColumn(right_table_->GetColumnName(i));
        }
        records_info_ = indexes::RecordsInfo::CreateFrom(*left_table_, *right_table_);
    }
    if (records_info_->GetLeftCompressor().GetNumberOfRecords() == 0 ||
        records_info_->GetRightCompressor().GetNumberOfRecords() == 0) {
        throw config::ConfigurationError("MD mining with either table empty is meaningless!");
    }
}

unsigned long long HyMD::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();
    SimilarityData::ColMatchesInfo column_matches_info;
    std::optional<util::WorkerThreadPool> pool_opt;
    util::WorkerThreadPool* pool_ptr = nullptr;
    if (threads_ > 1) {
        pool_opt.emplace(threads_);
        pool_ptr = &*pool_opt;
    }
    for (auto const& [left_column_name, right_column_name, creator] : column_matches_option_) {
        column_matches_info.emplace_back(creator->MakeMeasure(pool_ptr),
                                         left_schema_->GetColumn(left_column_name)->GetIndex(),
                                         right_schema_->GetColumn(right_column_name)->GetIndex());
    }
    std::size_t const column_match_number = column_matches_info.size();
    assert(column_match_number != 0);
    // TODO: make infrastructure for depth level
    SimilarityData similarity_data =
            SimilarityData::CreateFrom(records_info_.get(), std::move(column_matches_info));
    lattice::MdLattice lattice{[](...) { return 1; }, similarity_data.GetLhsIds(),
                               prune_nondisjoint_, max_cardinality_,
                               similarity_data.CreateMaxRhs()};
    LatticeTraverser lattice_traverser{
            std::make_unique<lattice::cardinality::MinPickingLevelGetter>(&lattice),
            {pool_ptr, records_info_.get(), similarity_data.GetColumnMatchesInfo(), min_support_,
             &lattice},
            pool_ptr};
    RecordPairInferrer record_pair_inferrer{&similarity_data, &lattice};

    bool done = false;
    do {
        done = record_pair_inferrer.InferFromRecordPairs(lattice_traverser.TakeRecommendations());
        done = lattice_traverser.TraverseLattice(done);
    } while (!done);

    RegisterResults(similarity_data, lattice.GetAll());

    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                 start_time)
            .count();
}

void HyMD::RegisterResults(SimilarityData const& similarity_data,
                           std::vector<lattice::MdLatticeNodeInfo> lattice_mds) {
    std::size_t const column_match_number = similarity_data.GetColumnMatchNumber();
    std::vector<model::md::ColumnMatch> column_matches;
    column_matches.reserve(column_match_number);
    for (Index column_match_index = 0; column_match_index < column_match_number;
         ++column_match_index) {
        auto [left_col_index, right_col_index] =
                similarity_data.GetColMatchIndices(column_match_index);
        column_matches.emplace_back(left_col_index, right_col_index,
                                    std::get<2>(column_matches_option_[column_match_index])
                                            ->GetSimilarityMeasureName());
    }
    std::vector<model::MD> mds;
    auto convert_lhs = [&](MdLhs const& lattice_lhs) {
        std::vector<model::md::LhsColumnSimilarityClassifier> lhs;
        lhs.reserve(column_match_number);
        Index lhs_index = 0;
        for (auto const& [child_index, ccv_id] : lattice_lhs) {
            for (Index lhs_limit = lhs_index + child_index; lhs_index != lhs_limit; ++lhs_index) {
                lhs.emplace_back(std::nullopt, lhs_index, kLowestBound);
            }
            assert(ccv_id != kLowestCCValueId);
            model::md::DecisionBoundary lhs_bound =
                    similarity_data.GetLhsDecisionBoundary(lhs_index, ccv_id);
            assert(lhs_bound != kLowestBound);
            model::md::DecisionBoundary max_disproved_bound =
                    similarity_data.GetDecisionBoundary(lhs_index, ccv_id - 1);
            lhs.emplace_back(max_disproved_bound == kLowestBound
                                     ? std::optional<model::md::DecisionBoundary>{std::nullopt}
                                     : max_disproved_bound,
                             lhs_index, lhs_bound);
            ++lhs_index;
        }
        for (; lhs_index != column_match_number; ++lhs_index) {
            lhs.emplace_back(std::nullopt, lhs_index, kLowestBound);
        }
        return lhs;
    };
    {
        assert(min_support_ <= records_info_->GetTotalPairsNum());
        // With the index approach all RHS in the lattice root are 0.
        auto empty_lhs = convert_lhs({column_match_number});
        for (Index rhs_index = 0; rhs_index != column_match_number; ++rhs_index) {
            model::md::DecisionBoundary rhs_bound =
                    similarity_data.GetDecisionBoundary(rhs_index, kLowestCCValueId);
            if (rhs_bound == kLowestBound) continue;
            model::md::ColumnSimilarityClassifier rhs{rhs_index, rhs_bound};
            mds.emplace_back(left_schema_.get(), right_schema_.get(), column_matches, empty_lhs,
                             rhs);
        }
    }
    for (lattice::MdLatticeNodeInfo const& md : lattice_mds) {
        std::vector<model::md::LhsColumnSimilarityClassifier> const lhs = convert_lhs(md.lhs);
        lattice::Rhs const& rhs = *md.rhs;
        for (Index rhs_index = 0; rhs_index != column_match_number; ++rhs_index) {
            ColumnClassifierValueId const rhs_value_id = rhs[rhs_index];
            if (rhs_value_id == kLowestCCValueId) continue;
            model::md::DecisionBoundary rhs_bound =
                    similarity_data.GetDecisionBoundary(rhs_index, rhs_value_id);
            model::md::ColumnSimilarityClassifier rhs{rhs_index, rhs_bound};
            mds.emplace_back(left_schema_.get(), right_schema_.get(), column_matches, lhs, rhs);
        }
    }
    std::sort(mds.begin(), mds.end(), utility::MdLess);
    for (model::MD const& md : mds) {
        RegisterMd(md);
    }
}

}  // namespace algos::hymd
