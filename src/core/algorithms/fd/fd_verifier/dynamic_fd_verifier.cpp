#include "algorithms/fd/fd_verifier/dynamic_fd_verifier.h"

#include <chrono>
#include <memory>
#include <stdexcept>

#include <easylogging++.h>

#include "config/equal_nulls/option.h"
#include "config/indices/option.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/crud_operations/operations.h"
#include "config/tabular_data/input_table/option.h"

namespace algos::fd_verifier {

DynamicFDVerifier::DynamicFDVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kLhsIndicesOpt.GetName(),
                          config::kRhsIndicesOpt.GetName()});
}

void DynamicFDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_schema_cols = [this]() { return input_table_->GetNumberOfColumns(); };

    auto check_inserts = [this](config::InputTable insert_batch) {
        if (insert_batch == nullptr || !insert_batch->HasNextRow()) {
            return;
        }
        if (insert_batch->GetNumberOfColumns() != input_table_->GetNumberOfColumns()) {
            throw config::ConfigurationError(
                    "Schema mismatch: insert statements must have the same number of columns as "
                    "the input table");
        }
        for (size_t i = 0; i < input_table_->GetNumberOfColumns(); ++i) {
            if (insert_batch->GetColumnName(i) != input_table_->GetColumnName(i)) {
                throw config::ConfigurationError(
                        "Schema mismatch: insert statements' column names must match the input "
                        "table");
            }
        }
    };

    auto check_deletes = [this](std::unordered_set<size_t> const& delete_batch) {
        if (delete_batch.empty()) {
            return;
        }
        for (size_t id : delete_batch) {
            if (!table_data_->IsRowIndexValid(id)) {
                throw config::ConfigurationError("Attempt to delete a non-existing row");
            }
        }
    };

    auto check_updates = [this](config::InputTable update_batch) {
        if (update_batch == nullptr || !update_batch->HasNextRow()) {
            return;
        }
        if (update_batch->GetNumberOfColumns() != input_table_->GetNumberOfColumns() + 1) {
            throw config::ConfigurationError(
                    "Schema mismatch: update statements must have the number of columns one more "
                    "than the input table");
        }
        for (size_t i = 0; i < input_table_->GetNumberOfColumns(); ++i) {
            if (update_batch->GetColumnName(i + 1) != input_table_->GetColumnName(i)) {
                throw config::ConfigurationError(
                        "Schema mismatch: update statements column names, except of first one, "
                        "must match the input table");
            }
        }
        std::unordered_set<size_t> rows_to_update;
        while (update_batch->HasNextRow()) {
            auto row = update_batch->GetNextRow();
            size_t id = std::stoull(row.front());
            if (!table_data_->IsRowIndexValid(id)) {
                throw config::ConfigurationError("Attempt to update a non-existing row");
            }
            if (rows_to_update.contains(id)) {
                throw config::ConfigurationError("Update statements have duplicates");
            }
            rows_to_update.emplace(id);
        }
        update_batch->Reset();
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(
            config::kInsertStatementsOpt(&insert_statements_table_).SetValueCheck(check_inserts));
    RegisterOption(
            config::kDeleteStatementsOpt(&delete_statement_indices_).SetValueCheck(check_deletes));
    RegisterOption(
            config::kUpdateStatementsOpt(&update_statements_table_).SetValueCheck(check_updates));
    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_schema_cols));
    RegisterOption(config::kRhsIndicesOpt(&rhs_indices_, get_schema_cols));
}

void DynamicFDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable(kCrudOptions);
}

void DynamicFDVerifier::LoadDataInternal() {
    CreateFD();
    input_table_->Reset();
    table_data_ = std::make_shared<model::DynamicTableData>(*input_table_);
    stats_calculator_ =
            std::make_unique<DynamicStatsCalculator>(table_data_, lhs_indices_, rhs_indices_);
    VerifyFD();
    SortHighlightsByProportionDescending();
}

unsigned long long DynamicFDVerifier::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();
    std::vector<std::pair<std::optional<size_t>, std::vector<int>>> lhs_inserts{}, rhs_inserts{};
    std::unordered_set<size_t> deletes_and_updates_indices{delete_statement_indices_};
    if (insert_statements_table_ != nullptr) {
        while (insert_statements_table_->HasNextRow()) {
            std::vector<std::string> row = insert_statements_table_->GetNextRow();
            if (row.size() != input_table_->GetNumberOfColumns()) {
                LOG(WARNING) << "Received row with size " << row.size() << ", but expected "
                             << input_table_->GetNumberOfColumns();
                continue;
            }
            lhs_inserts.emplace_back(std::nullopt, ParseRowForPLI(row.begin(), lhs_indices_));
            rhs_inserts.emplace_back(std::nullopt, ParseRowForPLI(row.begin(), rhs_indices_));
        }
        insert_statements_table_->Reset();
    }
    if (update_statements_table_ != nullptr) {
        while (update_statements_table_->HasNextRow()) {
            std::vector<std::string> row = update_statements_table_->GetNextRow();
            if (row.size() != input_table_->GetNumberOfColumns() + 1) {
                LOG(WARNING) << "Received row with size " << row.size() << ", but expected "
                             << input_table_->GetNumberOfColumns() + 1;
                continue;
            }
            size_t row_id = std::stoull(row.front());
            lhs_inserts.emplace_back(row_id, ParseRowForPLI(row.begin() + 1, lhs_indices_));
            rhs_inserts.emplace_back(row_id, ParseRowForPLI(row.begin() + 1, rhs_indices_));
            deletes_and_updates_indices.emplace(row_id);
        }
        update_statements_table_->Reset();
    }

    lhs_pli_->UpdateWith(lhs_inserts, deletes_and_updates_indices);
    rhs_pli_->UpdateWith(rhs_inserts, std::move(deletes_and_updates_indices));

    table_data_->Update(insert_statements_table_, update_statements_table_,
                        delete_statement_indices_);

    VerifyFD();
    SortHighlightsByProportionDescending();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void DynamicFDVerifier::VerifyFD() const {
    std::unique_ptr<model::DynPLI const> intersection_pli = lhs_pli_->Intersect(rhs_pli_.get());
    // stats calculator has init state after every Execute() call, will be fixed soon
    if (lhs_pli_->GetNumCluster() == intersection_pli->GetNumCluster()) {
        return;
    }

    stats_calculator_->CalculateStatistics(lhs_pli_.get(), rhs_pli_.get());
}

void DynamicFDVerifier::CreateFD() {
    size_t const num_columns = input_table_->GetNumberOfColumns();
    std::vector<std::vector<int>> lhs_rows, rhs_rows;
    std::vector<std::string> row;

    while (input_table_->HasNextRow()) {
        row = input_table_->GetNextRow();
        if (row.size() != num_columns) {
            continue;
        }
        lhs_rows.emplace_back(ParseRowForPLI(row.begin(), lhs_indices_));
        rhs_rows.emplace_back(ParseRowForPLI(row.begin(), rhs_indices_));
    }

    lhs_pli_ = model::DynPLI::CreateFor(lhs_rows);
    rhs_pli_ = model::DynPLI::CreateFor(rhs_rows);
}

std::vector<int> DynamicFDVerifier::ParseRowForPLI(
        model::IDatasetStream::Row::iterator const& row_begin,
        std::vector<unsigned int> const& indices) {
    std::vector<int> result{};
    for (size_t index : indices) {
        std::string const& field = *(row_begin + index);
        if (field.empty()) {
            result.emplace_back(kNullValueId);
        } else {
            auto [iter, is_value_new] = value_dictionary_.try_emplace(field, next_value_id_);
            if (is_value_new) {
                next_value_id_++;
            }
            result.emplace_back(iter->second);
        }
    }
    return result;
}

void DynamicFDVerifier::SortHighlightsByProportionAscending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(
            DynamicStatsCalculator::CompareHighlightsByProportionAscending());
}

void DynamicFDVerifier::SortHighlightsByProportionDescending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(
            DynamicStatsCalculator::CompareHighlightsByProportionDescending());
}

void DynamicFDVerifier::SortHighlightsByNumAscending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(DynamicStatsCalculator::CompareHighlightsByNumAscending());
}

void DynamicFDVerifier::SortHighlightsByNumDescending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(DynamicStatsCalculator::CompareHighlightsByNumDescending());
}

void DynamicFDVerifier::SortHighlightsBySizeAscending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(DynamicStatsCalculator::CompareHighlightsBySizeAscending());
}

void DynamicFDVerifier::SortHighlightsBySizeDescending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(DynamicStatsCalculator::CompareHighlightsBySizeDescending());
}

}  // namespace algos::fd_verifier
