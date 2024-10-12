#include "algorithms/md/hymd/hymd.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <ranges>

#include "algorithms/md/hymd/lattice/cardinality/min_picking_level_getter.h"
#include "algorithms/md/hymd/lattice/md_lattice.h"
#include "algorithms/md/hymd/lattice/single_level_func.h"
#include "algorithms/md/hymd/lattice_traverser.h"
#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/levenshtein_similarity_measure.h"
#include "algorithms/md/hymd/record_pair_inferrer.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/hymd/utility/inverse_permutation.h"
#include "algorithms/md/hymd/utility/md_less.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/thread_number/option.h"
#include "model/index.h"
#include "model/table/column.h"
#include "util/get_preallocated_vector.h"
#include "util/worker_thread_pool.h"

namespace {
using namespace algos::hymd;
using model::Index;

// Encapsulates "if using more than one thread, create a thread pool, otherwise pass nullptr to
// indicate single-threaded execution". Could have been a unique_ptr, but I don't want to use the
// heap when I don't have to.
struct PoolHolder {
private:
    std::optional<util::WorkerThreadPool> pool_holder_;
    util::WorkerThreadPool* pool_ptr_;

public:
    PoolHolder() : pool_holder_(), pool_ptr_(nullptr) {}

    PoolHolder(config::ThreadNumType threads) : pool_holder_(threads), pool_ptr_(&*pool_holder_) {}

    util::WorkerThreadPool* GetPtr() noexcept {
        return pool_ptr_;
    }
};

lattice::SingleLevelFunc GetLevelDefinitionFunc(LevelDefinition definition_enum) {
    // TODO: make infrastructure for depth level.
    // TODO: use depth level and validate several levels depending on thread number.
    switch (definition_enum) {
        case +LevelDefinition::cardinality:
            return [](...) { return 1; };
        case +LevelDefinition::lattice:
            return {nullptr};
        default:
            DESBORDANTE_ASSUME(false);
    }
}
}  // namespace

namespace algos::hymd {

HyMD::HyMD() : MdAlgorithm({}) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kLeftTable, kRightTable});
}

void HyMD::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kMinSupport, kPruneNonDisjoint, kColumnMatches, kMaxCardinality, kThreads,
                          kLevelDefinition});
}

void HyMD::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto min_support_default = [this]() {
        std::size_t const assumed_guaranteed_support =
                records_info_->OneTableGiven()
                        // Not always true, but usually is in practice if everything is matched to
                        // itself and the similarity measure isn't unusual.
                        ? records_info_->GetLeftCompressor().GetRecords().size()
                        : 0;
        return assumed_guaranteed_support + 1;
    };
    auto min_support_check = [this](std::size_t const& min_sup_value) {
        std::size_t const total_pairs_num = records_info_->GetTotalPairsNum();
        if (min_sup_value > total_pairs_num)
            throw config::ConfigurationError(
                    std::string("Support (") + std::to_string(min_sup_value) +
                    ") is greater than the number of pairs ( " + std::to_string(total_pairs_num) +
                    ") , mining MDs will be meaningless!");
    };

    static constexpr model::md::DecisionBoundary kDefaultDecisionBoundary = 0.7;
    auto column_matches_default = [this]() {
        Measures column_matches_option;
        if (records_info_->OneTableGiven()) {
            std::size_t const num_columns = left_schema_->GetNumColumns();
            column_matches_option.reserve(num_columns);
            for (Index i = 0; i != num_columns; ++i) {
                column_matches_option.push_back(
                        std::make_shared<
                                preprocessing::similarity_measure::LevenshteinSimilarityMeasure>(
                                i, i, kDefaultDecisionBoundary));
            }
        } else {
            std::size_t const num_columns_left = left_schema_->GetNumColumns();
            std::size_t const num_columns_right = right_schema_->GetNumColumns();
            column_matches_option.reserve(num_columns_left * num_columns_right);
            for (Index i = 0; i != num_columns_left; ++i) {
                for (Index j = 0; j != num_columns_right; ++j) {
                    column_matches_option.push_back(
                            std::make_shared<preprocessing::similarity_measure::
                                                     LevenshteinSimilarityMeasure>(
                                    i, j, kDefaultDecisionBoundary));
                }
            }
        }
        return column_matches_option;
    };
    auto column_matches_check = [this](Measures const& col_matches) {
        if (col_matches.empty())
            throw config::ConfigurationError("Mining with empty column matches is meaningless.");
        for (auto const& measure : col_matches) {
            measure->SetParameters(*left_schema_, *right_schema_);
        }
    };

    auto not_null = [](config::InputTable const& table) {
        if (table == nullptr) throw config::ConfigurationError("Left table may not be null.");
    };

    RegisterOption(Option{&right_table_, kRightTable, kDRightTable, config::InputTable{nullptr}});
    RegisterOption(Option{&left_table_, kLeftTable, kDLeftTable}
                           .SetValueCheck(std::move(not_null))
                           .SetConditionalOpts({{{}, {kRightTable}}}));

    RegisterOption(
            Option{&min_support_, kMinSupport, kDMinSupport, {std::move(min_support_default)}}
                    .SetValueCheck(std::move(min_support_check)));
    RegisterOption(Option{&prune_nondisjoint_, kPruneNonDisjoint, kDPruneNonDisjoint, true});
    RegisterOption(Option(&column_matches_option_, kColumnMatches, kDColumnMatches,
                          {std::move(column_matches_default)})
                           .SetValueCheck(std::move(column_matches_check)));
    RegisterOption(Option{&max_cardinality_, kMaxCardinality, kDMaxCardinality,
                          std::numeric_limits<std::size_t>::max()});
    RegisterOption(config::kThreadNumberOpt(&threads_));
    RegisterOption(Option{&level_definition_, kLevelDefinition, kDLevelDefinition,
                          +LevelDefinition::cardinality});
}

void HyMD::ResetStateMd() {}

void HyMD::LoadDataInternal() {
    left_schema_ = std::make_shared<RelationalSchema>(left_table_->GetRelationName());
    std::size_t const left_table_cols = left_table_->GetNumberOfColumns();
    for (Index i = 0; i != left_table_cols; ++i) {
        left_schema_->AppendColumn(left_table_->GetColumnName(i));
    }

    if (right_table_ == nullptr) {
        right_schema_ = left_schema_;

        records_info_ = indexes::RecordsInfo::CreateFrom(*left_table_);
    } else {
        right_schema_ = std::make_unique<RelationalSchema>(right_table_->GetRelationName());
        std::size_t const right_table_cols = right_table_->GetNumberOfColumns();
        for (Index i = 0; i != right_table_cols; ++i) {
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

    auto pool_holder = threads_ > 1 ? PoolHolder{threads_} : PoolHolder{};

    auto [similarity_data, short_sampling_enable] = SimilarityData::CreateFrom(
            records_info_.get(), column_matches_option_, pool_holder.GetPtr());

    lattice::MdLattice lattice{GetLevelDefinitionFunc(level_definition_),
                               similarity_data.GetLhsIdsInfo(), prune_nondisjoint_,
                               max_cardinality_, similarity_data.CreateMaxRhs()};

    auto [record_pair_inferrer, algorithm_finished] = RecordPairInferrer::Create(
            &lattice, records_info_.get(), &similarity_data.GetColumnMatchesInfo(),
            similarity_data.GetLhsIdsInfo(), std::move(short_sampling_enable),
            pool_holder.GetPtr());

    lattice::cardinality::MinPickingLevelGetter level_getter{&lattice};
    LatticeTraverser lattice_traverser{
            level_getter,
            {pool_holder.GetPtr(), records_info_.get(), similarity_data.GetColumnMatchesInfo(),
             min_support_, &lattice},
            pool_holder.GetPtr()};
    algorithm_finished = lattice_traverser.TraverseLattice(algorithm_finished);

    while (!algorithm_finished) {
        algorithm_finished =
                record_pair_inferrer.InferFromRecordPairs(lattice_traverser.TakeRecommendations());
        algorithm_finished = lattice_traverser.TraverseLattice(algorithm_finished);
    }

    RegisterResults(similarity_data, lattice.GetAll());

    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                 start_time)
            .count();
}

// Only serves to name parts of HyMD::RegisterResults.
class HyMD::RegisterHelper {
private:
    std::shared_ptr<RelationalSchema>& left_schema_;
    std::shared_ptr<RelationalSchema>& right_schema_;
    Measures const& column_matches_option_;
    SimilarityData const& similarity_data_;
    std::size_t const column_match_number_ = similarity_data_.GetColumnMatchNumber();
    std::size_t const trivial_column_match_number_ = similarity_data_.GetTrivialColumnMatchNumber();
    std::vector<Index> const& sorted_to_original_ = similarity_data_.GetIndexMapping();
    std::size_t const total_column_matches = column_matches_option_.size();

    std::shared_ptr<std::vector<model::md::ColumnMatch>> column_matches_ = MakeColumnMatchesPtr();

    std::vector<model::MD> mds_;

    std::shared_ptr<std::vector<model::md::ColumnMatch>> MakeColumnMatchesPtr() {
        std::vector<model::md::ColumnMatch> column_matches =
                util::GetPreallocatedVector<model::md::ColumnMatch>(total_column_matches);

        for (SimilarityData::MeasurePtr const& measure_ptr : column_matches_option_) {
            auto [left_column_index, right_column_index] = measure_ptr->GetIndices();
            column_matches.emplace_back(left_column_index, right_column_index,
                                        measure_ptr->GetName());
        }

        return std::make_shared<std::vector<model::md::ColumnMatch>>(std::move(column_matches));
    }

    std::vector<model::md::LhsColumnSimilarityClassifier> ConvertLhs(
            MdLhs const& lattice_lhs) const {
        std::vector<model::md::LhsColumnSimilarityClassifier> lhs =
                util::GetPreallocatedVector<model::md::LhsColumnSimilarityClassifier>(
                        total_column_matches);

        Index lhs_index = 0;
        for (auto const& [child_index, lhs_ccv_id] : lattice_lhs) {
            for (Index lhs_limit = lhs_index + child_index; lhs_index != lhs_limit; ++lhs_index) {
                lhs.emplace_back(std::nullopt, sorted_to_original_[lhs_index], kLowestBound);
            }
            assert(ccv_id != kLowestCCValueId);
            model::md::DecisionBoundary const lhs_bound =
                    similarity_data_.GetLhsDecisionBoundary(lhs_index, lhs_ccv_id);
            assert(lhs_bound != kLowestBound);
            model::md::DecisionBoundary const max_disproved_bound =
                    similarity_data_.GetLhsDecisionBoundary(lhs_index, lhs_ccv_id - 1);
            if (max_disproved_bound == kLowestBound)
                lhs.emplace_back(std::nullopt, sorted_to_original_[lhs_index], lhs_bound);
            else
                lhs.emplace_back(max_disproved_bound, sorted_to_original_[lhs_index], lhs_bound);
            ++lhs_index;
        }
        for (; lhs_index != column_match_number_; ++lhs_index) {
            lhs.emplace_back(std::nullopt, sorted_to_original_[lhs_index], kLowestBound);
        }
        for (auto const& [_, cm_index] : similarity_data_.GetTrivialInfo()) {
            lhs.emplace_back(std::nullopt, cm_index, kLowestBound);
        }
        utility::InversePermutation(
                total_column_matches, [&](model::Index i) { return lhs[i].GetColumnMatchIndex(); },
                [&](model::Index i, model::Index j) { std::swap(lhs[i], lhs[j]); });
        return lhs;
    }

public:
    RegisterHelper(HyMD& hymd, SimilarityData const& similarity_data)
        : left_schema_(hymd.left_schema_),
          right_schema_(hymd.right_schema_),
          column_matches_option_(hymd.column_matches_option_),
          similarity_data_(similarity_data) {}

    void AddEmptyLhsMds() {
        // All RHSs in the lattice root are 0 at this point.
        auto empty_lhs = ConvertLhs({});

        for (Index rhs_index : std::views::iota(0ul, column_match_number_)) {
            model::md::DecisionBoundary const rhs_bound =
                    similarity_data_.GetDecisionBoundary(rhs_index, kLowestCCValueId);
            if (rhs_bound == kLowestBound) continue;
            model::md::ColumnSimilarityClassifier rhs{sorted_to_original_[rhs_index], rhs_bound};
            mds_.emplace_back(left_schema_, right_schema_, column_matches_, empty_lhs, rhs);
        }
        for (auto const& [rhs_bound, cm_index] : similarity_data_.GetTrivialInfo()) {
            if (rhs_bound == kLowestBound) continue;
            model::md::ColumnSimilarityClassifier rhs{cm_index, rhs_bound};
            mds_.emplace_back(left_schema_, right_schema_, column_matches_, empty_lhs, rhs);
        }
    }

    void Add(std::vector<lattice::MdLatticeNodeInfo> const& lattice_mds) {
        for (lattice::MdLatticeNodeInfo const& md : lattice_mds) {
            std::vector<model::md::LhsColumnSimilarityClassifier> const lhs = ConvertLhs(md.lhs);
            lattice::Rhs const& rhs = md.node->rhs;
            for (Index rhs_index : std::views::iota(0ul, column_match_number_)) {
                ColumnClassifierValueId const rhs_value_id = rhs[rhs_index];
                if (rhs_value_id == kLowestCCValueId) continue;
                model::md::DecisionBoundary rhs_bound =
                        similarity_data_.GetDecisionBoundary(rhs_index, rhs_value_id);
                model::md::ColumnSimilarityClassifier rhs{sorted_to_original_[rhs_index],
                                                          rhs_bound};
                mds_.emplace_back(left_schema_, right_schema_, column_matches_, lhs, rhs);
            }
        }
    }

    void SortMds() {
        std::sort(mds_.begin(), mds_.end(), utility::MdLess);
    }

    std::vector<model::MD> const& GetMds() {
        return mds_;
    }
};

void HyMD::RegisterResults(SimilarityData const& similarity_data,
                           std::vector<lattice::MdLatticeNodeInfo> lattice_mds) {
    RegisterHelper register_helper{*this, similarity_data};

    register_helper.AddEmptyLhsMds();

    register_helper.Add(lattice_mds);

    register_helper.SortMds();
    for (model::MD const& md : register_helper.GetMds()) {
        RegisterMd(md);
    }
}

}  // namespace algos::hymd
