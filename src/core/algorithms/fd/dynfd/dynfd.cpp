#include "dynfd.h"

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
#include "util/timed_invoke.h"

namespace algos::dynfd {

void DynFD::ExecuteHyFD() {
    auto hy_fd_relation = ColumnLayoutRelationData::CreateFrom(*input_table_, true);

    auto&& [plis, pli_records, og_mapping] = hy::Preprocess(hy_fd_relation.get());
    auto plis_shared = std::make_shared<hy::PLIs>(std::move(plis));
    auto const pli_records_shared = std::make_shared<hy::Rows>(std::move(pli_records));

    hyfd::Sampler sampler(plis_shared, pli_records_shared);

    auto hyfd_positive_cover = std::make_shared<model::FDTree>(hy_fd_relation->GetNumColumns());
    hyfd::Inductor inductor(hyfd_positive_cover);
    hyfd::Validator validator(hyfd_positive_cover, plis_shared, pli_records_shared, threads_num_);

    hy::IdPairs comparison_suggestions;

    while (true) {
        auto non_fds = sampler.GetNonFDs(comparison_suggestions);

        inductor.UpdateFdTree(non_fds);

        comparison_suggestions = validator.ValidateAndExtendCandidates();

        if (comparison_suggestions.empty()) {
            break;
        }
    }

    for (size_t rhs = 0; rhs < hy_fd_relation->GetNumColumns(); ++rhs) {
        positive_cover_tree_->Remove(boost::dynamic_bitset(hy_fd_relation->GetNumColumns()), rhs);
    }
    for (RawFD const& fd : hyfd_positive_cover->FillFDs()) {
        positive_cover_tree_->AddFD(
                hy::RestoreAgreeSet(fd.lhs_, og_mapping, hy_fd_relation->GetNumColumns()),
                og_mapping[fd.rhs_]);
    }
}

void DynFD::MineFDs() {
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
}

unsigned long long DynFD::ExecuteInternal() {
    return util::TimedInvoke(&DynFD::MineFDs, this);
}

void DynFD::CoverInversion() {
    for (size_t i = 0; i < relation_->GetNumColumns(); i++) {
        boost::dynamic_bitset<> lhs(relation_->GetNumColumns());
        lhs.set();
        lhs.reset(i);
        negative_cover_tree_->AddNonFD(lhs, i, std::nullopt);
    }

    for (auto&& [lhs, rhs] : positive_cover_tree_->FillFDs()) {
        std::vector<boost::dynamic_bitset<>> violated =
                negative_cover_tree_->GetNonFdAndSpecials(lhs, rhs);
        for (auto const& non_fd : violated) {
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
}

void DynFD::LoadDataInternal() {
    relation_ = DynamicRelationData::CreateFrom(input_table_);
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error(
                "Got an empty dataset: FD mining is meaningless. If you want to specify columns, "
                "insert their names");
    }
    positive_cover_tree_ = std::make_shared<model::FDTree>(relation_->GetNumColumns());

    if (!relation_->Empty()) {
        ExecuteHyFD();
    }

    negative_cover_tree_ = std::make_shared<NonFDTree>(relation_->GetNumColumns());

    CoverInversion();

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
