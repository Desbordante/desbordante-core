#include "dynfd.h"

#include <easylogging++.h>

#include "algo_factory.h"
#include "algorithms/fd/hycommon/all_column_combinations.h"
#include "algorithms/fd/hycommon/preprocessor.h"
#include "algorithms/fd/hycommon/util/pli_util.h"
#include "algorithms/fd/hyfd/inductor.h"
#include "algorithms/fd/hyfd/sampler.h"
#include "algorithms/fd/hyfd/validator.h"
#include "algorithms/fd/raw_fd.h"
#include "indices/option.h"
#include "option_using.h"
#include "tabular_data/crud_operations/delete/option.h"
#include "tabular_data/crud_operations/insert/option.h"
#include "tabular_data/crud_operations/operations.h"
#include "tabular_data/crud_operations/update/option.h"
#include "tabular_data/input_table/option.h"

namespace algos::dynfd {

void DynFD::ExecuteHyFD() {
    std::shared_ptr hy_fd_relation = ColumnLayoutRelationData::CreateFrom(*input_table_, true);

    auto [plis, pli_records, og_mapping] = hy::Preprocess(hy_fd_relation.get());
    auto plis_shared = std::make_shared<hy::PLIs>(std::move(plis));
    auto const pli_records_shared = std::make_shared<hy::Rows>(std::move(pli_records));

    hyfd::Sampler sampler(plis_shared, pli_records_shared);

    auto positive_cover_tree = std::make_shared<model::FDTree>(hy_fd_relation->GetNumColumns());
    hyfd::Inductor inductor(positive_cover_tree);
    hyfd::Validator validator(positive_cover_tree, plis_shared, pli_records_shared, threads_num_);

    hy::IdPairs comparison_suggestions;

    while (true) {
        auto non_fds = sampler.GetNonFDs(comparison_suggestions);

        inductor.UpdateFdTree(std::move(non_fds));

        comparison_suggestions = validator.ValidateAndExtendCandidates();

        if (comparison_suggestions.empty()) {
            break;
        }

        LOG(TRACE) << "Cycle done";
    }

    for (size_t rhs = 0; rhs < hy_fd_relation->GetNumColumns(); ++rhs) {
        positive_cover_tree_->Remove(boost::dynamic_bitset(hy_fd_relation->GetNumColumns()), rhs);
    }
    for (auto fd : positive_cover_tree->FillFDs()) {
        fd.lhs_ = hy::RestoreAgreeSet(fd.lhs_, og_mapping, hy_fd_relation->GetNumColumns());
        fd.rhs_ = og_mapping[fd.rhs_];
        positive_cover_tree_->AddFD(fd.lhs_, fd.rhs_);
    }
}

unsigned long long DynFD::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();

    bool const is_non_fd_validation_needed =
            (!delete_statement_indices_.empty()) || (update_statements_table_ != nullptr);
    bool const is_fd_validation_needed =
            (update_statements_table_ != nullptr) || (insert_statements_table_ != nullptr);

    if (!delete_statement_indices_.empty()) {
        relation_->DeleteBatch(delete_statement_indices_);
    }
    if (update_statements_table_ != nullptr) {
        relation_->DeleteRecordsFromUpdateBatch(update_statements_table_);
    }

    if (is_non_fd_validation_needed) {
        validator_->ValidateNonFds();
    }

    size_t const first_insert_batch_id = relation_->GetNextRecordId();
    if (update_statements_table_ != nullptr) {
        relation_->InsertRecordsFromUpdateBatch(update_statements_table_);
    }
    if (insert_statements_table_ != nullptr) {
        relation_->InsertBatch(insert_statements_table_);
    }

    if (is_fd_validation_needed) {
        validator_->ValidateFds(first_insert_batch_id);
    }

    SetProgress(kTotalProgressPercent);
    RegisterFDs(positive_cover_tree_->FillFDs());
    auto const elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void DynFD::LoadDataInternal() {
    relation_ = DynamicRelationData::CreateFrom(input_table_);
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error(
                "Got an empty dataset: FD mining is meaningless. If you want to specify columns, "
                "insert their names");
    }
    positive_cover_tree_ = std::make_shared<model::FDTree>(GetRelation().GetNumColumns());

    if (!relation_->Empty()) {
        ExecuteHyFD();
    }

    negative_cover_tree_ = std::make_shared<NonFDTree>(GetRelation().GetNumColumns());

    // Cover inversion
    for (size_t i = 0; i < relation_->GetNumColumns(); i++) {
        boost::dynamic_bitset<> lhs(relation_->GetNumColumns());
        lhs.set();
        lhs.reset(i);
        negative_cover_tree_->AddNonFD(lhs, i, std::nullopt);
    }

    for (auto&& [lhs, rhs] : positive_cover_tree_->FillFDs()) {
        std::vector<boost::dynamic_bitset<>> violated = negative_cover_tree_->GetSpecials(lhs, rhs);
        for (auto&& non_fd : violated) {
            negative_cover_tree_->Remove(non_fd, rhs);
            for (size_t bit = lhs.find_first(); bit != boost::dynamic_bitset<>::npos;
                 bit = lhs.find_next(bit)) {
                boost::dynamic_bitset<> new_lhs = non_fd;
                new_lhs.reset(bit);
                if (!negative_cover_tree_->ContainsNonFdOrSpecial(new_lhs, rhs)) {
                    negative_cover_tree_->AddNonFD(new_lhs, rhs, std::nullopt);
                }
            }
        }
    }

    validator_ = std::make_shared<Validator>(positive_cover_tree_, negative_cover_tree_, relation_);
}

void DynFD::MakeExecuteOptsAvailableFDInternal() {
    using namespace config::names;
    MakeOptionsAvailable(kCrudOptions);
}

void DynFD::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_inserts = [this](config::InputTable const& insert_batch) {
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
        for (size_t const id : delete_batch) {
            if (!relation_->IsRowIndexValid(id)) {
                throw config::ConfigurationError("Attempt to delete a non-existing row");
            }
        }
    };

    auto check_updates = [this](config::InputTable const& update_batch) {
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
            if (!relation_->IsRowIndexValid(id)) {
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
    RegisterOption(config::kThreadNumberOpt(&threads_num_));
}

void DynFD::RegisterFDs(std::vector<RawFD>&& fds) {
    auto const schema = GetRelation().GetSharedPtrSchema();
    for (auto&& [lhs, rhs] : fds) {
        Vertical lhs_v(schema.get(), lhs);
        Column rhs_c(schema.get(), schema->GetColumn(rhs)->GetName(), rhs);
        RegisterFd(std::move(lhs_v), std::move(rhs_c), schema);
    }
}

DynFD::DynFD() : FDAlgorithm({kDefaultPhaseName}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

DynamicRelationData const& DynFD::GetRelation() const {
    assert(relation_ != nullptr);
    return *relation_;
}

}  // namespace algos::dynfd
