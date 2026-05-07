#include "core/algorithms/dc/weever/weever.h"

#include <algorithm>
#include <cassert>
#include <chrono>

#include "core/algorithms/dc/model/column_operand.h"
#include "core/algorithms/dc/parser/dc_parser.h"
#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/crud_operations/delete/option.h"
#include "core/config/tabular_data/crud_operations/insert/option.h"
#include "core/config/tabular_data/crud_operations/operations.h"
#include "core/config/tabular_data/crud_operations/update/option.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/model/table/dynamic_data/typed_dynamic_row_table_data.h"
#include "core/model/table/dynamic_data/update_strategies/separate_update_strategy.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "core/util/logger.h"

namespace algos {

void Weever::ResetState() {}

void Weever::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable(kCrudOptions);
}

Weever::Weever() {
    RegisterOptions();
}

void Weever::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option<std::string>(&dc_string_, kDenialConstraint, kDDenialConstraint, ""));
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kInsertStatementsOpt(&insert_statements_table_));
    RegisterOption(config::kDeleteStatementsOpt(&delete_statement_indices_));
    RegisterOption(config::kUpdateStatementsOpt(&update_statements_table_));

    MakeOptionsAvailable({kDenialConstraint, config::names::kTable});
}

void Weever::LoadDataInternal() {
    relation_ = std::make_unique<RelationalSchema>(input_table_->GetRelationName());
    for (size_t i = 0; i < input_table_->GetNumberOfColumns(); ++i) {
        relation_->AppendColumn(input_table_->GetColumnName(i));
    }

    // Mirror DCVerifier's GetIndexOffset(): +1 for 1-based IDs, +1 for a header row.
    auto* parser = dynamic_cast<CSVParser const*>(input_table_.get());
    bool has_header = parser ? parser->HasHeader() : false;
    index_offset_ = 1 + static_cast<size_t>(has_header);

    data_ = std::make_unique<model::TypedDynamicRowTableData>(*input_table_, index_offset_);
    table_index_ = dc::TableIndex(data_.get());
}

void Weever::BuildIndices(dc::DC const& dc) {
    std::vector<dc::Predicate> const& preds = dc.GetPredicates();

    // Create indices for each predicate
    for (auto const& pred : preds) {
        dc::ColumnOperand l_operand = pred.GetLeftOperand();
        dc::ColumnOperand r_operand = pred.GetRightOperand();
        dc::OperatorType op = pred.GetOperator().GetType();

        model::ColumnIndex l_col = l_operand.GetColumnIndex();
        model::ColumnIndex r_col = r_operand.GetColumnIndex();

        // For each column involved in the predicate, create appropriate index if not exists
        table_index_.CreateIndex(l_col, op);
        table_index_.CreateIndex(r_col, op);
    }

    // Index is left empty here; Preprocess populates it row-by-row so that
    // each InsertTuple call sees only the rows inserted before it.
}

void Weever::Preprocess() {
    try {
        dc::DCParser parser = dc::DCParser(dc_string_, relation_.get(), data_->GetTypes());
        dc_ = parser.Parse();
    } catch (std::exception const& e) {
        LOG_INFO("[Weever] DC parse error: {}", e.what());
        return;
    }

    dc_.Canonize();
    BuildIndices(dc_);

    for (size_t row_id : data_->GetAllIds()) {
        InsertTuple(row_id);
        table_index_.InsertRow(row_id);
    }
}

unsigned long long int Weever::ExecuteInternal() {
    auto start = std::chrono::system_clock::now();

    data_->SetStrategy(std::make_unique<model::SeparateUpdateStrategy>(
            insert_statements_table_, update_statements_table_, delete_statement_indices_));

    if (!is_preprocessed_) {
        Preprocess();
        is_preprocessed_ = true;
    }

    Update();

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start);

    return elapsed_time.count();
}

void Weever::Update() {
    // Update dynamic table data according to strategy
    data_->Update();

    auto const& deleted = data_->GetDeleted();
    auto const& inserted = data_->GetInserted();
    auto const& updated = data_->GetUpdated();

    // Process deletions first: remove from index immediately so subsequent
    // CheckTupleAgainstDC calls don't see stale entries for deleted tuples
    for (auto const& [tuple_id, old_data] : deleted) {
        DeleteTuple(tuple_id);
        table_index_.DeleteRow(tuple_id, old_data.GetData());
    }

    // Process insertions: check violations against the current index state,
    // then add the new tuple so subsequent insertions can check against it
    for (size_t tuple_id : inserted) {
        InsertTuple(tuple_id);
        table_index_.InsertRow(tuple_id);
    }

    // Process updates: remove old data from index, update violations, then
    // check the new data and add it — same incremental pattern as above
    for (auto const& [tuple_id, old_data] : updated) {
        DeleteTuple(tuple_id);
        table_index_.DeleteRow(tuple_id, old_data.GetData());

        InsertTuple(tuple_id);
        table_index_.InsertRow(tuple_id);
    }
}

void Weever::DeleteTuple(size_t tuple_id) {
    auto inv_it = inv_violations_.find(tuple_id);
    if (inv_it != inv_violations_.end()) {
        for (uint32_t x : inv_it->second) {
            violations_[x].remove(static_cast<uint32_t>(tuple_id));
        }
        inv_violations_.erase(inv_it);
    }

    auto v_it = violations_.find(tuple_id);
    if (v_it != violations_.end()) {
        for (uint32_t y : v_it->second) {
            inv_violations_[y].remove(static_cast<uint32_t>(tuple_id));
        }
        violations_.erase(v_it);
    }
}

void Weever::InsertTuple(size_t new_tuple_id) {
    assert(data_->RowExists(new_tuple_id) &&
           "[Weever] InsertTuple: tuple_id does not exist in data_");

    roaring::Roaring s_candidates = table_index_.GetIndexedIds();
    roaring::Roaring t_candidates = table_index_.GetIndexedIds();

    auto const& new_row = data_->GetRow(new_tuple_id);
    auto const& types = data_->GetTypes();

    // Process each predicate to refine the intermediates (Section 5.1 predicate processing)
    for (auto const& pred : dc_.GetPredicates()) {
        model::ColumnIndex l_col = pred.GetLeftOperand().GetColumnIndex();
        model::ColumnIndex r_col = pred.GetRightOperand().GetColumnIndex();

        dc::OperatorType op = pred.GetOperator().GetType();

        // Obtain operand based on predicate type (Section 5.1)
        roaring::Roaring operand_l, operand_r;

        if (op == dc::OperatorType::kEqual or op == dc::OperatorType::kUnequal) {
            dc::Component val_r = {new_row[r_col], types[r_col]};
            auto const& eq_index_r = table_index_.GetEqualityIndex(r_col);
            if (eq_index_r.count(val_r)) {
                operand_r = eq_index_r.at(val_r);
            }

            dc::Component val_l = {new_row[l_col], types[l_col]};
            auto const& eq_index_l = table_index_.GetEqualityIndex(l_col);
            if (eq_index_l.count(val_l)) {
                operand_l = eq_index_l.at(val_l);
            }

            if (op == dc::OperatorType::kEqual) {
                s_candidates &= operand_l;
                t_candidates &= operand_r;
            } else {
                s_candidates -= operand_l;
                t_candidates -= operand_r;
            }
        } else {
            // Predicate: s.l_col OP t.r_col
            // s_candidates holds candidates for (new=s, existing=t): keep t where new.l_col OP
            // t.r_col t_candidates holds candidates for (existing=s, new=t): keep s where s.l_col
            // OP new.r_col
            dc::Component val_l = {new_row[l_col], types[l_col]};
            auto const& lt_index_l = table_index_.GetLessThanIndex(l_col);

            dc::Component val_r = {new_row[r_col], types[r_col]};
            auto const& lt_index_r = table_index_.GetLessThanIndex(r_col);

            roaring::Roaring operand_l;
            roaring::Roaring operand_r;

            if (op == dc::OperatorType::kLess) {
                // want t.r_col > val_l: remove where r_col <= val_l
                s_candidates -= lt_index_r.FindLessOrEqual(val_l);
                // want s.l_col < val_r: keep where l_col < val_r
                t_candidates &= lt_index_l.FindLess(val_r);
            } else if (op == dc::OperatorType::kLessEqual) {
                // want t.r_col >= val_l: remove where r_col < val_l
                operand_l = lt_index_r.FindLess(val_l);
                s_candidates -= operand_l;
                // want s.l_col <= val_r: keep where l_col <= val_r
                operand_r = lt_index_l.FindLessOrEqual(val_r);
                t_candidates &= operand_r;
            } else if (op == dc::OperatorType::kGreater) {
                // want t.r_col < val_l: keep where r_col < val_l
                s_candidates &= lt_index_r.FindLess(val_l);
                // want s.l_col > val_r: remove where l_col <= val_r
                t_candidates -= lt_index_l.FindLessOrEqual(val_r);
            } else {  // kGreaterEqual
                // want t.r_col <= val_l: keep where r_col <= val_l
                operand_l = lt_index_r.FindLessOrEqual(val_l);
                s_candidates &= operand_l;
                // want s.l_col >= val_r: remove where l_col < val_r
                operand_r = lt_index_l.FindLess(val_r);
                t_candidates -= operand_r;
            }
        }

        // Early termination if both intermediates are empty
        if (s_candidates.isEmpty() and t_candidates.isEmpty()) {
            break;
        }
    }

    for (uint32_t s_cand : s_candidates) {
        violations_[new_tuple_id].add(s_cand);
        inv_violations_[s_cand].add(static_cast<uint32_t>(new_tuple_id));
    }
    for (uint32_t t_cand : t_candidates) {
        violations_[t_cand].add(static_cast<uint32_t>(new_tuple_id));
        inv_violations_[new_tuple_id].add(t_cand);
    }
}

std::vector<dc::Violation> Weever::GetViolations() const {
    std::vector<dc::Violation> result;
    for (auto const& [first_id, bitmap] : violations_) {
        for (uint32_t second_id : bitmap) {
            result.push_back({first_id, second_id});
        }
    }

    return result;
}

size_t Weever::GetViolationsSize() const {
    size_t res = 0;
    for (auto const& [_, bitmap] : violations_) {
        res += bitmap.cardinality();
    }
    return res;
}

}  // namespace algos
