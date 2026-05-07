// Tests for TypedDynamicRowTableData.
//
// NOTE: All tables in this file use the same 3-column schema {Int, Double, String}.
// This is required to work around the static row_size bug in AppendRow: the row buffer
// size is computed once from the first AppendRow call and reused across all instances,
// so mixing schemas in a single test run would corrupt memory.

#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "core/model/table/dynamic_data/typed_dynamic_row_table_data.h"
#include "core/model/table/idataset_stream.h"
#include "core/model/types/builtin.h"
#include "core/model/types/type.h"

namespace tests {
namespace {

class MockDatasetStream : public model::IDatasetStream {
public:
    MockDatasetStream(std::vector<std::string> col_names,
                      std::vector<std::vector<std::string>> rows)
        : col_names_(std::move(col_names)), rows_(std::move(rows)), pos_(0) {}

    Row GetNextRow() override {
        return rows_[pos_++];
    }

    bool HasNextRow() const override {
        return pos_ < rows_.size();
    }

    size_t GetNumberOfColumns() const override {
        return col_names_.size();
    }

    std::string GetColumnName(size_t index) const override {
        return col_names_.at(index);
    }

    std::string GetRelationName() const override {
        return "test";
    }

    void Reset() override {
        pos_ = 0;
    }

private:
    std::vector<std::string> col_names_;
    std::vector<std::vector<std::string>> rows_;
    size_t pos_;
};

// Standard 3-column schema used by all tests: {Int, Double, String}.
static std::vector<std::string> const kCols{"id", "score", "label"};
static std::vector<std::vector<std::string>> const kRows{
        {"1", "1.5", "alice"},
        {"2", "2.5", "bob"},
        {"3", "3.5", "charlie"},
};
static std::vector<std::string> const kNewRow{"4", "4.5", "dave"};
static std::vector<std::string> const kUpdatedRow{"99", "9.9", "zara"};

// Helper: returns the single ID present in `after` but not in `before`.
size_t FindNewId(std::set<size_t> const& before, std::set<size_t> const& after) {
    for (size_t id : after) {
        if (!before.count(id)) return id;
    }
    // Should never happen in a well-behaved test.
    return std::numeric_limits<size_t>::max();
}

}  // namespace

// ====================== CONSTRUCTION ======================

TEST(TypedDynamicRowTableData, GetNumCols) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    EXPECT_EQ(table.GetNumCols(), 3u);
}

TEST(TypedDynamicRowTableData, GetNumRows_AfterConstruction) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    EXPECT_EQ(table.GetNumRows(), 3u);
}

// ====================== EMPTY TABLE ======================

TEST(TypedDynamicRowTableData, EmptyTable_ZeroRows) {
    MockDatasetStream stream(kCols, {});
    model::TypedDynamicRowTableData table(stream);
    EXPECT_EQ(table.GetNumRows(), 0u);
}

TEST(TypedDynamicRowTableData, EmptyTable_ColumnCountPreserved) {
    MockDatasetStream stream(kCols, {});
    model::TypedDynamicRowTableData table(stream);
    EXPECT_EQ(table.GetNumCols(), 3u);
}

TEST(TypedDynamicRowTableData, EmptyTable_AllIdsIsEmpty) {
    MockDatasetStream stream(kCols, {});
    model::TypedDynamicRowTableData table(stream);
    EXPECT_TRUE(table.GetAllIds().empty());
}

TEST(TypedDynamicRowTableData, EmptyTable_TrackingSetsAreEmpty) {
    MockDatasetStream stream(kCols, {});
    model::TypedDynamicRowTableData table(stream);
    EXPECT_TRUE(table.GetInserted().empty());
    EXPECT_TRUE(table.GetUpdated().empty());
    EXPECT_TRUE(table.GetDeleted().empty());
}

// ====================== TYPE INFERENCE ======================

TEST(TypedDynamicRowTableData, GetTypes_ReturnsThreeTypes) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    EXPECT_EQ(table.GetTypes().size(), 3u);
}

TEST(TypedDynamicRowTableData, TypeInference_IntColumn) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    EXPECT_EQ(table.GetTypes()[0]->GetTypeId(), +model::TypeId::kInt);
}

TEST(TypedDynamicRowTableData, TypeInference_DoubleColumn) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    EXPECT_EQ(table.GetTypes()[1]->GetTypeId(), +model::TypeId::kDouble);
}

TEST(TypedDynamicRowTableData, TypeInference_StringColumn) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    EXPECT_EQ(table.GetTypes()[2]->GetTypeId(), +model::TypeId::kString);
}

// ====================== ROW EXISTS ======================

TEST(TypedDynamicRowTableData, RowExists_TrueForAllInitialRows) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    for (size_t i = 0; i < kRows.size(); ++i) {
        EXPECT_TRUE(table.RowExists(i)) << "Expected row " << i << " to exist";
    }
}

TEST(TypedDynamicRowTableData, RowExists_FalseForOutOfRangeId) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    EXPECT_FALSE(table.RowExists(100));
}

// ====================== GET ALL IDs ======================

TEST(TypedDynamicRowTableData, GetAllIds_ContainsAllInitialRowIds) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    std::set<size_t> const expected{0, 1, 2};
    EXPECT_EQ(table.GetAllIds(), expected);
}

// ====================== INITIAL TRACKING SETS ======================

TEST(TypedDynamicRowTableData, InitialTrackingSets_AreAllEmpty) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    EXPECT_TRUE(table.GetInserted().empty());
    EXPECT_TRUE(table.GetUpdated().empty());
    EXPECT_TRUE(table.GetDeleted().empty());
}

// ====================== GET VALUE ======================

TEST(TypedDynamicRowTableData, GetValue_IntColumnFirstRow) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    std::byte const* val = table.GetValue(0, 0);
    EXPECT_EQ(model::Type::GetValue<model::Int>(val), 1);
}

TEST(TypedDynamicRowTableData, GetValue_DoubleColumnFirstRow) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    std::byte const* val = table.GetValue(0, 1);
    EXPECT_DOUBLE_EQ(model::Type::GetValue<model::Double>(val), 1.5);
}

TEST(TypedDynamicRowTableData, GetValue_StringColumnFirstRow) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    std::byte const* val = table.GetValue(0, 2);
    EXPECT_EQ(model::Type::GetValue<model::String>(val), "alice");
}

TEST(TypedDynamicRowTableData, GetValue_IntColumnAllRows) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    for (size_t r = 0; r < kRows.size(); ++r) {
        auto val = model::Type::GetValue<model::Int>(table.GetValue(r, 0));
        EXPECT_EQ(val, static_cast<model::Int>(r + 1)) << "Row " << r;
    }
}

TEST(TypedDynamicRowTableData, GetValue_StringColumnAllRows) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    std::vector<std::string> const expected_names{"alice", "bob", "charlie"};
    for (size_t r = 0; r < kRows.size(); ++r) {
        auto val = model::Type::GetValue<model::String>(table.GetValue(r, 2));
        EXPECT_EQ(val, expected_names[r]) << "Row " << r;
    }
}

// ====================== GET ROW ======================

TEST(TypedDynamicRowTableData, GetRow_ReturnsCorrectNumberOfValues) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    EXPECT_EQ(table.GetRow(0).size(), 3u);
}

TEST(TypedDynamicRowTableData, GetRow_ValuesMatchGetValue) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const row = table.GetRow(1);
    ASSERT_EQ(row.size(), 3u);
    EXPECT_EQ(row[0], table.GetValue(1, 0));
    EXPECT_EQ(row[1], table.GetValue(1, 1));
    EXPECT_EQ(row[2], table.GetValue(1, 2));
}

TEST(TypedDynamicRowTableData, GetRow_CorrectTypedValues) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const row = table.GetRow(2);
    EXPECT_EQ(model::Type::GetValue<model::Int>(row[0]), 3);
    EXPECT_DOUBLE_EQ(model::Type::GetValue<model::Double>(row[1]), 3.5);
    EXPECT_EQ(model::Type::GetValue<model::String>(row[2]), "charlie");
}

// ====================== GET COL ======================

TEST(TypedDynamicRowTableData, GetCol_ReturnsCorrectSize) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    EXPECT_EQ(table.GetCol(0).size(), kRows.size());
    EXPECT_EQ(table.GetCol(1).size(), kRows.size());
    EXPECT_EQ(table.GetCol(2).size(), kRows.size());
}

TEST(TypedDynamicRowTableData, GetCol_IntColumnValues) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const col = table.GetCol(0);
    ASSERT_EQ(col.size(), 3u);
    // Column contains IDs 1, 2, 3 (in all_ids_ order = 0, 1, 2 → values 1, 2, 3).
    std::vector<model::Int> actual;
    for (std::byte const* p : col) actual.push_back(model::Type::GetValue<model::Int>(p));
    std::sort(actual.begin(), actual.end());
    EXPECT_EQ(actual, (std::vector<model::Int>{1, 2, 3}));
}

TEST(TypedDynamicRowTableData, GetCol_DoubleColumnValues) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const col = table.GetCol(1);
    ASSERT_EQ(col.size(), 3u);
    std::vector<model::Double> actual;
    for (std::byte const* p : col) actual.push_back(model::Type::GetValue<model::Double>(p));
    std::sort(actual.begin(), actual.end());
    EXPECT_EQ(actual, (std::vector<model::Double>{1.5, 2.5, 3.5}));
}

TEST(TypedDynamicRowTableData, GetCol_StringColumnValues) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const col = table.GetCol(2);
    ASSERT_EQ(col.size(), 3u);
    std::vector<std::string> actual;
    for (std::byte const* p : col) actual.push_back(model::Type::GetValue<model::String>(p));
    std::sort(actual.begin(), actual.end());
    EXPECT_EQ(actual, (std::vector<std::string>{"alice", "bob", "charlie"}));
}

TEST(TypedDynamicRowTableData, GetCol_AfterDelete_SizeDecreases) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(1);
    EXPECT_EQ(table.GetCol(0).size(), 2u);
}

TEST(TypedDynamicRowTableData, GetCol_AfterDelete_RemovedValueAbsent) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(1);  // removes row with id=2, score=2.5, label="bob"
    auto const col = table.GetCol(0);
    for (std::byte const* p : col) {
        EXPECT_NE(model::Type::GetValue<model::Int>(p), 2) << "Deleted row value still in column";
    }
}

// ====================== DELETE ROW ======================

TEST(TypedDynamicRowTableData, DeleteRow_RowNoLongerExists) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(1);
    EXPECT_FALSE(table.RowExists(1));
}

TEST(TypedDynamicRowTableData, DeleteRow_OtherRowsStillExist) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(1);
    EXPECT_TRUE(table.RowExists(0));
    EXPECT_TRUE(table.RowExists(2));
}

TEST(TypedDynamicRowTableData, DeleteRow_DecreasesRowCount) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    size_t const before = table.GetNumRows();
    table.DeleteRow(0);
    EXPECT_EQ(table.GetNumRows(), before - 1);
}

TEST(TypedDynamicRowTableData, DeleteRow_AddsToDeletedSet) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(2);
    EXPECT_EQ(table.GetDeleted().count(2), 1u);
}

TEST(TypedDynamicRowTableData, DeleteRow_DoesNotAffectInsertedOrUpdatedSets) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(0);
    EXPECT_TRUE(table.GetInserted().empty());
    EXPECT_TRUE(table.GetUpdated().empty());
}

TEST(TypedDynamicRowTableData, DeleteRow_RemovesFromAllIds) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(1);
    auto const ids = table.GetAllIds();
    EXPECT_EQ(ids.count(1), 0u);
    EXPECT_EQ(ids.size(), 2u);
}

TEST(TypedDynamicRowTableData, MultipleDeletes_ReduceRowCountCorrectly) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(0);
    table.DeleteRow(2);
    EXPECT_EQ(table.GetNumRows(), 1u);
}

TEST(TypedDynamicRowTableData, MultipleDeletes_AllDeletedIdsTracked) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(0);
    table.DeleteRow(1);
    auto const& deleted = table.GetDeleted();
    EXPECT_EQ(deleted.count(0), 1u);
    EXPECT_EQ(deleted.count(1), 1u);
}

TEST(TypedDynamicRowTableData, DeleteAllRows_EmptiesTable) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(0);
    table.DeleteRow(1);
    table.DeleteRow(2);
    EXPECT_EQ(table.GetNumRows(), 0u);
    EXPECT_TRUE(table.GetAllIds().empty());
}

TEST(TypedDynamicRowTableData, DeleteAllRows_AllIdsTrackedAsDeleted) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    for (size_t i = 0; i < kRows.size(); ++i) table.DeleteRow(i);
    auto const& deleted = table.GetDeleted();
    for (size_t i = 0; i < kRows.size(); ++i) {
        EXPECT_EQ(deleted.count(i), 1u) << "Row " << i << " not in deleted set";
    }
}

// ====================== APPEND ROW ======================

TEST(TypedDynamicRowTableData, AppendRow_IncreasesRowCount) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    size_t const before = table.GetNumRows();
    table.AppendRow(kNewRow);
    EXPECT_EQ(table.GetNumRows(), before + 1);
}

TEST(TypedDynamicRowTableData, AppendRow_NewRowExists) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const ids_before = table.GetAllIds();
    table.AppendRow(kNewRow);
    size_t const new_id = FindNewId(ids_before, table.GetAllIds());
    EXPECT_TRUE(table.RowExists(new_id));
}

TEST(TypedDynamicRowTableData, AppendRow_AddsToInsertedSet) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const ids_before = table.GetAllIds();
    table.AppendRow(kNewRow);
    size_t const new_id = FindNewId(ids_before, table.GetAllIds());
    EXPECT_EQ(table.GetInserted().count(new_id), 1u);
}

TEST(TypedDynamicRowTableData, AppendRow_AddsToAllIds) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const ids_before = table.GetAllIds();
    table.AppendRow(kNewRow);
    size_t const new_id = FindNewId(ids_before, table.GetAllIds());
    EXPECT_EQ(table.GetAllIds().count(new_id), 1u);
}

TEST(TypedDynamicRowTableData, AppendRow_ValuesAreCorrect) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const ids_before = table.GetAllIds();
    table.AppendRow(kNewRow);
    size_t const new_id = FindNewId(ids_before, table.GetAllIds());
    EXPECT_EQ(model::Type::GetValue<model::Int>(table.GetValue(new_id, 0)), 4);
    EXPECT_DOUBLE_EQ(model::Type::GetValue<model::Double>(table.GetValue(new_id, 1)), 4.5);
    EXPECT_EQ(model::Type::GetValue<model::String>(table.GetValue(new_id, 2)), "dave");
}

TEST(TypedDynamicRowTableData, AppendRow_DoesNotAffectUpdatedOrDeletedSets) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.AppendRow(kNewRow);
    EXPECT_TRUE(table.GetUpdated().empty());
    EXPECT_TRUE(table.GetDeleted().empty());
}

TEST(TypedDynamicRowTableData, MultipleAppends_RowCountGrowsCorrectly) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.AppendRow({"4", "4.5", "dave"});
    table.AppendRow({"5", "5.5", "eve"});
    EXPECT_EQ(table.GetNumRows(), kRows.size() + 2);
}

TEST(TypedDynamicRowTableData, MultipleAppends_IdsAreDistinct) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const ids_before = table.GetAllIds();
    table.AppendRow({"4", "4.5", "dave"});
    size_t const id1 = FindNewId(ids_before, table.GetAllIds());
    auto const ids_mid = table.GetAllIds();
    table.AppendRow({"5", "5.5", "eve"});
    size_t const id2 = FindNewId(ids_mid, table.GetAllIds());
    EXPECT_NE(id1, id2);
    EXPECT_TRUE(table.RowExists(id1));
    EXPECT_TRUE(table.RowExists(id2));
}

TEST(TypedDynamicRowTableData, MultipleAppends_AllInsertedIdsTracked) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const ids_before = table.GetAllIds();
    table.AppendRow({"4", "4.5", "dave"});
    size_t const id1 = FindNewId(ids_before, table.GetAllIds());
    auto const ids_mid = table.GetAllIds();
    table.AppendRow({"5", "5.5", "eve"});
    size_t const id2 = FindNewId(ids_mid, table.GetAllIds());
    auto const& inserted = table.GetInserted();
    EXPECT_EQ(inserted.count(id1), 1u);
    EXPECT_EQ(inserted.count(id2), 1u);
}

// ====================== UPDATE ROW ======================

TEST(TypedDynamicRowTableData, UpdateRow_ChangesIntValue) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.UpdateRow(0, kUpdatedRow);
    EXPECT_EQ(model::Type::GetValue<model::Int>(table.GetValue(0, 0)), 99);
}

TEST(TypedDynamicRowTableData, UpdateRow_ChangesDoubleValue) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.UpdateRow(0, kUpdatedRow);
    EXPECT_DOUBLE_EQ(model::Type::GetValue<model::Double>(table.GetValue(0, 1)), 9.9);
}

TEST(TypedDynamicRowTableData, UpdateRow_ChangesStringValue) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.UpdateRow(0, kUpdatedRow);
    EXPECT_EQ(model::Type::GetValue<model::String>(table.GetValue(0, 2)), "zara");
}

TEST(TypedDynamicRowTableData, UpdateRow_AddsToUpdatedSet) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.UpdateRow(1, kUpdatedRow);
    EXPECT_EQ(table.GetUpdated().count(1), 1u);
}

TEST(TypedDynamicRowTableData, UpdateRow_RowStillExists) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.UpdateRow(2, kUpdatedRow);
    EXPECT_TRUE(table.RowExists(2));
}

TEST(TypedDynamicRowTableData, UpdateRow_DoesNotChangeRowCount) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    size_t const before = table.GetNumRows();
    table.UpdateRow(0, kUpdatedRow);
    EXPECT_EQ(table.GetNumRows(), before);
}

TEST(TypedDynamicRowTableData, UpdateRow_DoesNotAffectInsertedOrDeletedSets) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.UpdateRow(0, kUpdatedRow);
    EXPECT_TRUE(table.GetInserted().empty());
    EXPECT_TRUE(table.GetDeleted().empty());
}

TEST(TypedDynamicRowTableData, UpdateRow_GetRowReflectsNewValues) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.UpdateRow(2, kUpdatedRow);
    auto const row = table.GetRow(2);
    ASSERT_EQ(row.size(), 3u);
    EXPECT_EQ(model::Type::GetValue<model::Int>(row[0]), 99);
    EXPECT_DOUBLE_EQ(model::Type::GetValue<model::Double>(row[1]), 9.9);
    EXPECT_EQ(model::Type::GetValue<model::String>(row[2]), "zara");
}

TEST(TypedDynamicRowTableData, UpdateRow_GetColReflectsNewValue) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.UpdateRow(0, kUpdatedRow);  // id column: 1 → 99
    auto const col = table.GetCol(0);
    bool found = false;
    for (std::byte const* p : col) {
        if (model::Type::GetValue<model::Int>(p) == 99) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Updated value not reflected in GetCol";
}

TEST(TypedDynamicRowTableData, MultipleUpdates_AllUpdatedIdsTracked) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.UpdateRow(0, kUpdatedRow);
    table.UpdateRow(2, kUpdatedRow);
    auto const& updated = table.GetUpdated();
    EXPECT_EQ(updated.count(0), 1u);
    EXPECT_EQ(updated.count(2), 1u);
}

TEST(TypedDynamicRowTableData, UpdateRow_DoesNotAffectOtherRows) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.UpdateRow(0, kUpdatedRow);
    // Row 1 should still have original values.
    EXPECT_EQ(model::Type::GetValue<model::Int>(table.GetValue(1, 0)), 2);
    EXPECT_DOUBLE_EQ(model::Type::GetValue<model::Double>(table.GetValue(1, 1)), 2.5);
    EXPECT_EQ(model::Type::GetValue<model::String>(table.GetValue(1, 2)), "bob");
}

// ====================== COMPLEX SCENARIOS ======================

TEST(TypedDynamicRowTableData, DeleteThenAppend_IdsAreDistinctFromDeleted) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(0);
    EXPECT_EQ(table.GetNumRows(), 2u);

    auto const ids_before = table.GetAllIds();
    table.AppendRow(kNewRow);
    size_t const new_id = FindNewId(ids_before, table.GetAllIds());

    EXPECT_TRUE(table.RowExists(new_id));
    EXPECT_FALSE(table.RowExists(0));
    EXPECT_EQ(table.GetNumRows(), 3u);
}

TEST(TypedDynamicRowTableData, DeleteThenAppend_BothSetsTracked) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(1);
    auto const ids_before = table.GetAllIds();
    table.AppendRow(kNewRow);
    size_t const new_id = FindNewId(ids_before, table.GetAllIds());

    EXPECT_EQ(table.GetDeleted().count(1), 1u);
    EXPECT_EQ(table.GetInserted().count(new_id), 1u);
}

TEST(TypedDynamicRowTableData, UpdateThenDelete_BothSetsTracked) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.UpdateRow(0, kUpdatedRow);
    table.DeleteRow(0);

    EXPECT_FALSE(table.RowExists(0));
    EXPECT_EQ(table.GetUpdated().count(0), 1u);
    EXPECT_EQ(table.GetDeleted().count(0), 1u);
}

TEST(TypedDynamicRowTableData, AppendThenUpdate_InsertAndUpdateBothTracked) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    auto const ids_before = table.GetAllIds();
    table.AppendRow(kNewRow);
    size_t const new_id = FindNewId(ids_before, table.GetAllIds());
    table.UpdateRow(new_id, kUpdatedRow);

    EXPECT_EQ(model::Type::GetValue<model::Int>(table.GetValue(new_id, 0)), 99);
    EXPECT_EQ(table.GetInserted().count(new_id), 1u);
    EXPECT_EQ(table.GetUpdated().count(new_id), 1u);
}

TEST(TypedDynamicRowTableData, GetAllIds_AfterMixedOperations) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    // initial: {0, 1, 2}
    table.DeleteRow(0);  // {1, 2}
    auto const ids_before = table.GetAllIds();
    table.AppendRow(kNewRow);  // {1, 2, new_id}
    size_t const new_id = FindNewId(ids_before, table.GetAllIds());

    std::set<size_t> const expected{1, 2, new_id};
    EXPECT_EQ(table.GetAllIds(), expected);
}

TEST(TypedDynamicRowTableData, DeleteAndUpdateSeparateRows_BothSetsIndependent) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(0);
    table.UpdateRow(2, kUpdatedRow);

    EXPECT_EQ(table.GetDeleted().count(0), 1u);
    EXPECT_EQ(table.GetUpdated().count(2), 1u);
    EXPECT_EQ(table.GetDeleted().count(2), 0u);
    EXPECT_EQ(table.GetUpdated().count(0), 0u);
}

TEST(TypedDynamicRowTableData, AppendMultiple_RowCountGrowsCorrectly) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.AppendRow({"4", "4.5", "dave"});
    table.AppendRow({"5", "5.5", "eve"});
    table.AppendRow({"6", "6.5", "frank"});
    EXPECT_EQ(table.GetNumRows(), kRows.size() + 3);
}

TEST(TypedDynamicRowTableData, DeleteMiddle_RemainingRowValuesUnchanged) {
    MockDatasetStream stream(kCols, kRows);
    model::TypedDynamicRowTableData table(stream);
    table.DeleteRow(1);
    EXPECT_EQ(model::Type::GetValue<model::Int>(table.GetValue(0, 0)), 1);
    EXPECT_EQ(model::Type::GetValue<model::String>(table.GetValue(0, 2)), "alice");
    EXPECT_EQ(model::Type::GetValue<model::Int>(table.GetValue(2, 0)), 3);
    EXPECT_EQ(model::Type::GetValue<model::String>(table.GetValue(2, 2)), "charlie");
}

}  // namespace tests
