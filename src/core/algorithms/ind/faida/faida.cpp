#include "faida.h"

#include "algorithms/ind/faida/candidate_generation/apriori_candidate_generator.h"
#include "algorithms/ind/faida/inclusion_testing/combined_inclusion_tester.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/thread_number/option.h"
#include "max_arity/option.h"
#include "model/table/column.h"
#include "util/logger.h"

namespace algos {

Faida::Faida() : INDAlgorithm({}) {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option{&sample_size_, kSampleSize, kDSampleSize, 500});
    RegisterOption(Option{&hll_accuracy_, kHllAccuracy, kDHllAccuracy, 0.001});
    RegisterOption(config::kMaxArityOpt(&max_arity_));
    RegisterOption(Option{&ignore_null_cols_, kIgnoreNullCols, kDIgnoreNullCols, false});
    RegisterOption(Option{&ignore_const_cols_, kIgnoreConstantCols, kDIgnoreConstantCols, false});
    RegisterOption(config::kThreadNumberOpt(&number_of_threads_));

    MakeOptionsAvailable({kSampleSize});
}

void Faida::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({config::kMaxArityOpt.GetName(), kHllAccuracy, kIgnoreNullCols,
                          kIgnoreConstantCols, config::kThreadNumberOpt.GetName()});
}

void Faida::LoadINDAlgorithmDataInternal() {
    auto start_time = std::chrono::system_clock::now();

    data_ = faida::Preprocessor::CreateHashedStores("Faida", input_tables_, sample_size_);

    auto const prep_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    prepr_time_ = prep_milliseconds.count();
    LOG_DEBUG("Preprocessing time: {}", prepr_time_);
}

void Faida::ResetINDAlgorithmState() {
    inclusion_tester_.reset();
    insert_time_ = 0;
    check_time_ = 0;
}

std::vector<std::shared_ptr<faida::SimpleCC>> Faida::CreateUnaryCCs(
        faida::Preprocessor const& data) const {
    std::vector<std::shared_ptr<SimpleCC>> combinations;

    constexpr int const index = 0;
    for (TableIndex table_idx = 0; table_idx < data.GetStores().size(); table_idx++) {
        AbstractColumnStore const& store = *data.GetStores()[table_idx];
        size_t const num_columns = store.GetSchema()->GetNumColumns();

        for (ColumnIndex col_idx = 0; col_idx < num_columns; col_idx++) {
            if (ignore_const_cols_ && store.IsConstantCol(col_idx)) {
                LOG_DEBUG("Ignoring constant column with idx {}", col_idx);
                continue;
            }
            if (ignore_null_cols_ && store.IsNullCol(col_idx)) {
                LOG_DEBUG("Ignoring null column with idx {}", col_idx);
                continue;
            }
            combinations.emplace_back(
                    std::make_shared<SimpleCC>(table_idx, std::vector{col_idx}, index));
        }
    }

    return combinations;
}

std::vector<faida::SimpleIND> Faida::CreateUnaryINDCandidates(
        std::vector<std::shared_ptr<SimpleCC>> const& combinations) const {
    std::vector<SimpleIND> candidates;
    candidates.reserve(combinations.size() * combinations.size());

    for (auto left_it = combinations.begin(); left_it != combinations.end(); left_it++) {
        for (auto right_it = combinations.begin(); right_it != combinations.end(); right_it++) {
            if (left_it != right_it) {
                candidates.emplace_back(*left_it, *right_it);
            }
        }
    }

    return candidates;
}

std::vector<std::shared_ptr<faida::SimpleCC>> Faida::ExtractCCs(
        std::vector<SimpleIND> const& candidates) const {
    std::unordered_set<std::shared_ptr<SimpleCC>> combinations;
    for (SimpleIND const& ind_candidate : candidates) {
        combinations.insert(ind_candidate.Left());
        combinations.insert(ind_candidate.Right());
    }
    return {combinations.begin(), combinations.end()};
}

unsigned long long Faida::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();
    size_t level_num = 0;

    inclusion_tester_ = std::make_unique<faida::CombinedInclusionTester>(
            number_of_threads_, hll_accuracy_, data_->GetNullHash());

    std::vector<std::shared_ptr<SimpleCC>> combinations = CreateUnaryCCs(*data_);
    std::vector<SimpleIND> candidates = CreateUnaryINDCandidates(combinations);
    LOG_DEBUG("\nCreated {} candidates", candidates.size());

    faida::IInclusionTester::ActiveColumns active_columns = inclusion_tester_->SetCCs(combinations);
    InsertRows(active_columns, *data_);

    std::vector<SimpleIND> last_result = TestCandidates(candidates);
    LOG_DEBUG("Found {} INDs on level {}", last_result.size(), level_num);
    RegisterInds(last_result);

    while (!last_result.empty() && ++level_num != max_arity_) {
        candidates = faida::apriori_candidate_generator::CreateCombinedCandidates(last_result);
        if (candidates.empty()) {
            LOG_DEBUG("\nNo candidates on level {}", level_num);
            break;
        }
        LOG_DEBUG("\nCreated {} candidates", candidates.size());

        /* All unique combinations on this level */
        combinations = ExtractCCs(candidates);
        LOG_DEBUG("Extracted {} CCs", combinations.size());

        active_columns = inclusion_tester_->SetCCs(combinations);
        InsertRows(active_columns, *data_);

        last_result = TestCandidates(candidates);
        RegisterInds(last_result);
        LOG_DEBUG("Found {} INDs on level {}", last_result.size(), level_num);
    }

    auto const elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    unsigned long long const millis = elapsed_milliseconds.count();

    LOG_DEBUG("\nCertain checks:\t{}", inclusion_tester_->GetNumCertainChecks());
    LOG_DEBUG("Uncertain checks:\t{}", inclusion_tester_->GetNumUncertainChecks());
    LOG_DEBUG("\nOverall time\t{}", millis + prepr_time_);
    LOG_DEBUG("Time (without preprocessing):\t{}", millis);
    LOG_DEBUG("\tInserting:\t{}", insert_time_);
    LOG_DEBUG("\tChecking:\t{}", check_time_);
    LOG_DEBUG("\nIND count:\t{}", INDList().size());

    return millis;
}

void Faida::InsertRows(faida::IInclusionTester::ActiveColumns const& active_columns,
                       faida::Preprocessor const& data) {
    using namespace faida;
    using std::vector;
    auto start_time = std::chrono::system_clock::now();

    vector<AbstractColumnStore::HashedTableSample> samples;
    samples.reserve(data.GetStores().size());

    for (std::unique_ptr<AbstractColumnStore> const& store : data.GetStores()) {
        samples.emplace_back(store->GetSample());
    }
    inclusion_tester_->Initialize(samples);

    for (auto const& [curr_table, columns] : active_columns) {
        AbstractColumnStore const& table_store = *data.GetStores()[curr_table];
        std::unique_ptr<IRowIterator> input_iter = table_store.GetRows(columns);

        inclusion_tester_->StartInsertRow(curr_table);
        size_t row_idx = 0;
        while (input_iter->HasNextBlock()) {
            IRowIterator::Block const& next_block = input_iter->GetNextBlock();
            size_t const block_size = input_iter->GetBlockSize();
            inclusion_tester_->InsertRows(next_block, block_size);
            row_idx += block_size;
        }
        LOG_DEBUG("Inserted {} rows from table {}", row_idx, curr_table);
    }

    inclusion_tester_->FinalizeInsertion();
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    size_t const millis = elapsed_milliseconds.count();
    insert_time_ += millis;
    LOG_DEBUG("Insert rows time:\t{}", millis);
}

std::vector<faida::SimpleIND> Faida::TestCandidates(std::vector<SimpleIND> const& candidates) {
    auto start_time = std::chrono::system_clock::now();
    std::vector<SimpleIND> result;

    for (SimpleIND const& candidate_ind : candidates) {
        if (inclusion_tester_->IsIncludedIn(candidate_ind.Left(), candidate_ind.Right())) {
            result.emplace_back(candidate_ind);
        }
    }

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    size_t const millis = elapsed_milliseconds.count();
    check_time_ += millis;
    LOG_DEBUG("Candidates check time:\t{}", millis);
    return result;
}

void Faida::RegisterInds(std::vector<SimpleIND> const& inds) {
    for (SimpleIND const& ind : inds) {
        RegisterIND(ind.Left(), ind.Right());
    }
}

}  // namespace algos
