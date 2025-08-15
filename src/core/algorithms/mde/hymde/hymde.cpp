#include "algorithms/mde/hymde/hymde.h"

#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lattice.h"
#include "algorithms/mde/hymde/cover_calculation/lattice_traverser.h"
#include "algorithms/mde/hymde/cover_calculation/minimal_selecting_level_getter.h"
#include "algorithms/mde/hymde/cover_calculation/record_pair_inferrer.h"
#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/record_match_indexes/preprocessing_result.h"
#include "algorithms/mde/hymde/utility/index_range.h"
#include "algorithms/mde/hymde/utility/zip.h"
#include "config/exceptions.h"
#include "config/names_and_descriptions.h"
#include "config/option.h"
#include "config/option_using.h"
#include "config/thread_number/option.h"
#include "model/index.h"
#include "util/get_preallocated_vector.h"
#include "util/worker_thread_pool.h"

namespace algos::hymde {
using model::Index;

HyMDE::HyMDE() : Algorithm({}) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kLeftTable, kRightTable});
}

void HyMDE::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable(
            {kMinSupport, kPruneNonDisjoint, kRecordMatches, kMaxCardinality, kThreads});
}

void HyMDE::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    using CalculatorCreatorPtr = record_match_indexes::PreprocessingResult::CalculatorCreatorPtr;

    auto min_support_default = [this]() {
        std::size_t const assumed_guaranteed_support =
                dictionary_compressed_records_->HoldsOneTable()
                        // Not always true, but usually is in practice if everything is matched to
                        // itself and the similarity measure isn't unusual.
                        ? dictionary_compressed_records_->GetLeftTable().size()
                        : 0;
        return dictionary_compressed_records_->GetTotalPairsCount() == 1
                       ? 1
                       : assumed_guaranteed_support + 1;
    };
    auto min_support_check = [this](std::size_t const& min_sup_value) {
        std::size_t const total_pairs_count = dictionary_compressed_records_->GetTotalPairsCount();
        if (min_sup_value > total_pairs_count)
            throw config::ConfigurationError(
                    std::string("Support (") + std::to_string(min_sup_value) +
                    ") is greater than the number of pairs (" + std::to_string(total_pairs_count) +
                    ") , mining MDEs will be meaningless!");
    };

    auto record_matches_check = [this](ComponentCalculationSpecification const& creators) {
        if (creators.empty())
            throw config::ConfigurationError("Mining with empty record matches is meaningless.");
        for (CalculatorCreatorPtr const& calculator_creator_ptr : creators) {
            calculator_creator_ptr->CheckSchemas(*left_schema_, *right_schema_);
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
    RegisterOption(Option(&component_calculation_specification_, kRecordMatches, kDRecordMatches)
                           .SetValueCheck(std::move(record_matches_check)));
    RegisterOption(Option{&max_cardinality_, kMaxCardinality, kDMaxCardinality,
                          std::numeric_limits<std::size_t>::max()});
    RegisterOption(config::kThreadNumberOpt(&threads_));
}

void HyMDE::ResetState() {
    search_space_specification_.clear();
    mde_specifications_.clear();
}

void HyMDE::LoadDataInternal() {
    left_schema_ = std::make_shared<RelationalSchema>(left_table_->GetRelationName());
    std::size_t const left_table_cols = left_table_->GetNumberOfColumns();
    for (Index i : utility::IndexRange(left_table_cols)) {
        left_schema_->AppendColumn(left_table_->GetColumnName(i));
    }

    if (right_table_ == nullptr) {
        right_schema_ = left_schema_;

        dictionary_compressed_records_ = records::DictionaryCompressed::Compress(*left_table_);
    } else {
        right_schema_ = std::make_unique<RelationalSchema>(right_table_->GetRelationName());
        std::size_t const right_table_cols = right_table_->GetNumberOfColumns();
        for (Index i : utility::IndexRange(right_table_cols)) {
            right_schema_->AppendColumn(right_table_->GetColumnName(i));
        }

        dictionary_compressed_records_ =
                records::DictionaryCompressed::Compress(*left_table_, *right_table_);
    }

    if (dictionary_compressed_records_->GetLeftTable().empty() ||
        dictionary_compressed_records_->GetRightTable().empty()) {
        throw config::ConfigurationError("MDE mining with either table empty is meaningless!");
    }
}

unsigned long long HyMDE::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();

    std::unique_ptr<util::WorkerThreadPool> pool_holder =
            threads_ > 1 ? std::make_unique<util::WorkerThreadPool>(threads_) : nullptr;

    // Preprocess.
    auto [record_matches, classifier_values, useful_record_matches, data_partition_index, indexes,
          rcv_id_lr_maps, assertions] =
            record_match_indexes::PreprocessingResult::Create(
                    *left_schema_, *right_schema_, *dictionary_compressed_records_,
                    component_calculation_specification_, pool_holder.get());

    // Create search space specification.
    std::size_t const record_matches_count = record_matches.size();
    std::vector<SearchSpaceFactorSpecification> search_space_specification =
            util::GetPreallocatedVector<SearchSpaceFactorSpecification>(record_matches_count);
    for (auto [record_match, rm_bounds] : utility::Zip(record_matches, classifier_values)) {
        search_space_specification.emplace_back(std::move(record_match),
                                                std::move(rm_bounds.values));
    }
    search_space_specification_ = std::move(search_space_specification);

    // Add 0 cardinality LHS MDEs to the result.
    std::vector<RecordClassifierIdentifiers> rhss;
    for (model::Index record_match_index : utility::IndexRange(classifier_values.size())) {
        bool total_is_universal =
                classifier_values[record_match_index].total_decision_boundary_is_universal_;
        if (!total_is_universal) {
            rhss.emplace_back(record_match_index, kLowestRCValueId);
        }
    }
    if (!rhss.empty()) {
        mde_specifications_.emplace_back(
                LhsSpecification{{}, dictionary_compressed_records_->GetTotalPairsCount()},
                std::move(rhss));
    }

    if (useful_record_matches.empty()) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now() - start_time)
                .count();
    }

    // Create starting RHS array
    std::size_t const useful_count = useful_record_matches.size();
    cover_calculation::lattice::Rhs max_rhs{useful_count};
    max_rhs.non_zero_count = useful_count;
    auto rhs_span = std::span{max_rhs.begin.get(), useful_count};
    for (auto [useful_rm_index, rcv_id] : utility::Zip(useful_record_matches, rhs_span)) {
        std::size_t const rm_classifier_values =
                search_space_specification_[useful_rm_index].decision_boundaries.size();
        DESBORDANTE_ASSUME(rm_classifier_values > 1);
        rcv_id = rm_classifier_values - 1;
    }

    cover_calculation::lattice::MdeLattice lattice{
            [](RecordClassifierValueId, model::Index) { return 1; }, rcv_id_lr_maps,
            prune_nondisjoint_, max_cardinality_, std::move(max_rhs)};

    auto [record_pair_inferrer, algorithm_finished] = cover_calculation::RecordPairInferrer::Create(
            &lattice, &data_partition_index, &indexes, rcv_id_lr_maps, std::move(assertions),
            pool_holder.get());

    cover_calculation::MinimalSelectingLevelGetter level_getter{&lattice};
    cover_calculation::BatchValidator validator{
            pool_holder.get(),
            &data_partition_index,
            indexes,
            min_support_,
            &lattice,
            rcv_id_lr_maps,
            {mde_specifications_, useful_record_matches, rcv_id_lr_maps}};
    cover_calculation::LatticeTraverser lattice_traverser{level_getter, validator,
                                                          pool_holder.get()};
    algorithm_finished = lattice_traverser.TraverseLattice(algorithm_finished);

    while (!algorithm_finished) {
        algorithm_finished =
                record_pair_inferrer.InferFromRecordPairs(validator.GetCurrentResults());
        algorithm_finished = lattice_traverser.TraverseLattice(algorithm_finished);
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                 start_time)
            .count();
}

CompactMDEStorage HyMDE::GetMdes() const {
    return {left_schema_, right_schema_, search_space_specification_, mde_specifications_};
}
}  // namespace algos::hymde
