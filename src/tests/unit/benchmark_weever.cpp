#include <array>
#include <chrono>
#include <cstdio>
#include <memory>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/dc/verifier/dc_verifier.h"
#include "core/algorithms/dc/weever/weever.h"
#include "core/config/names.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/idataset_stream.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "tests/common/all_csv_configs.h"
#include "tests/mock/mock_input_table.h"

namespace tests {

using namespace algos;
using namespace config::names;

namespace {

// ── DC and schema shared by all tests ────────────────────────────────────────

static std::string const kStrictDC =
        "!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate)";

static std::vector<std::string> const kColumns = {"State", "Salary", "FedTaxRate"};

// ── Utility: convert integer k to a 3-decimal tax-rate string ────────────────
// FormatTax(1)    → "0.001"
// FormatTax(1000) → "1.000"
static std::string FormatTax(size_t k) {
    std::string s = std::to_string(k);
    while (s.size() < 4) s = "0" + s;
    return s.substr(0, s.size() - 3) + "." + s.substr(s.size() - 3);
}

// ── Utility: apply one Execute() cycle with the given CRUD payload ────────────
static void ApplyAndExecute(Weever& weever, config::InputTable inserts = {},
                            std::unordered_set<size_t> deletes = {},
                            config::InputTable updates = {}) {
    weever.SetOption(kInsertStatements, std::move(inserts));
    weever.SetOption(kDeleteStatements, std::move(deletes));
    weever.SetOption(kUpdateStatements, std::move(updates));
    weever.Execute();
}

}  // namespace

TEST(WeeeverBenchmark, EmptyInsert) {
    static constexpr size_t kNumTuples = 50'000;
    static constexpr size_t kStep = 5'000;

    algos::StdParamsMap params = {
            {kCsvConfig, kTestDC1},
            {kDenialConstraint, kStrictDC},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    weever->Execute();

    // TestDC1 with strict inequalities has no violations.
    ApplyAndExecute(*weever, {}, {2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
    ASSERT_TRUE(weever->GetViolations().empty()) << "Table must be empty before benchmark starts";

    std::printf("step,weever_batch_us\n");
    std::fflush(stdout);

    for (size_t batch = 0; batch < kNumTuples / kStep; ++batch) {
        size_t k = (batch + 1) * kStep;

        std::vector<model::IDatasetStream::Row> batchRows;
        batchRows.reserve(kStep);
        for (size_t j = 1; j <= kStep; ++j) {
            size_t rowNum = batch * kStep + j;
            batchRows.push_back({"Texas", std::to_string(kNumTuples - rowNum), FormatTax(rowNum)});
        }

        auto insertTable = std::make_shared<MockTable>("insert", kColumns, batchRows);
        auto t1 = std::chrono::steady_clock::now();
        ApplyAndExecute(*weever, std::move(insertTable));
        auto t2 = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);

        auto v_size = weever->GetViolationsSize();
        std::printf("table size: %zu, time: %ld ms, v_size: %ld\n", k, ms.count(), v_size);
        std::fflush(stdout);
    }
}

TEST(DCVerifierBenchmark, EmptyInsert) {
    static constexpr size_t kStep = 1'000;
    static constexpr size_t kMaxRows = 10'000;

    std::printf("step,dcverifier_us\n");
    std::fflush(stdout);

    std::vector<model::IDatasetStream::Row> currentRows;
    currentRows.reserve(kMaxRows);

    for (size_t k = kStep; k <= kMaxRows; k += kStep) {
        size_t prevSize = currentRows.size();
        for (size_t rowNum = prevSize + 1; rowNum <= k; ++rowNum) {
            currentRows.push_back({"Texas", std::to_string(kMaxRows - rowNum), FormatTax(rowNum)});
        }

        auto table = std::make_shared<MockTable>("table", kColumns, currentRows);
        config::InputTable tInput = table;
        algos::StdParamsMap dcParams = {
                {kTable, tInput},
                {kDenialConstraint, kStrictDC},
                {kDoCollectViolations, true},
        };
        auto verifier = algos::CreateAndLoadAlgorithm<DCVerifier>(dcParams);

        auto t1 = std::chrono::steady_clock::now();
        verifier->Execute();
        auto t2 = std::chrono::steady_clock::now();
        long long dcv_us = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

        auto v_size = 0l;
        std::printf("table size: %zu, time: %lld ms, v_size: %ld\n", k, dcv_us, v_size);
        std::fflush(stdout);
    }
}

class WeeverTaxBenchmark : public ::testing::Test {
public:
    std::string const phi1 = "!(s.State == t.State and s.Salary > t.Salary and s.Rate < t.Rate)";
    std::string const phi2 =
            "!(s.AreaCode == t.AreaCode and s.Phone == t.Phone and s.Salary > t.Salary)";
    std::string const phi3 =
            "!(s.FName == t.FName and s.LName == t.LName and s.Gender != t.Gender)";

    std::string const dc = phi2;

    std::vector<std::string> const tax_cols = {
            "FName",  "LName", "Gender",      "AreaCode",      "Phone",
            "City",   "State", "Zip",         "MaritalStatus", "HasChild",
            "Salary", "Rate",  "SingleExemp", "MarriedExemp",  "ChildExemp"};

    // std::vector<std::string> const tax_cols = {"State", "Zip"};
    size_t index_offset = 2;
    size_t batch_size = 1'000;
    size_t num_batches = 1'000;
    std::unique_ptr<Weever> weever;
    std::vector<model::IDatasetStream::Row> tax_rows;

    void SetUp() {
        algos::StdParamsMap params = {
                {kCsvConfig, kTax},
                {kDenialConstraint, dc},
        };

        std::printf("# DC: %s\n", dc.c_str());
        std::printf("# batch_size=%zu  num_batches=%zu\n\n", batch_size, num_batches);
        std::fflush(stdout);

        // Load Tax rows for use as insert/update data source
        auto parser = std::make_shared<CSVParser>(kTax);
        while (parser->HasNextRow()) tax_rows.push_back(parser->GetNextRow());

        weever = algos::CreateAndLoadAlgorithm<Weever>(params);
        std::cout << "Load finished" << std::endl;

        auto t0 = std::chrono::steady_clock::now();

        weever->Execute();  // initial load of Tax
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - t0)
                               .count();
        // for (auto const& [f, s] : weever->GetViolations()) {
        //     LOG_INFO("({}, {})", f, s);
        // }
        std::printf("time: %lld ms, init vc size: %ld\n", ms, weever->GetViolationsSize());
        std::fflush(stdout);
    }
};

TEST_F(WeeverTaxBenchmark, BatchInsert) {
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<size_t> row_dist(0, tax_rows.size() - 1);

    std::printf("\n# Tax batch insert benchmark\n");
    std::fflush(stdout);
    for (size_t b = 0; b < num_batches; ++b) {
        std::vector<model::IDatasetStream::Row> rows;
        rows.reserve(batch_size);
        for (size_t i = 0; i < batch_size; ++i) {
            rows.push_back(tax_rows[row_dist(rng)]);
        }
        auto ins_tbl = std::make_shared<MockTable>("epic_batch", tax_cols, std::move(rows));

        weever->SetOption(kInsertStatements, config::InputTable(std::move(ins_tbl)));
        weever->SetOption(kDeleteStatements, std::unordered_set<size_t>{});
        weever->SetOption(kUpdateStatements, config::InputTable{});

        auto t0 = std::chrono::steady_clock::now();
        weever->Execute();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0);

        if ((b + 1) % 5 == 0) {
            std::printf("(%zu, %ld) ", b + 1, ms.count());
        }
        if ((b + 1) % 25 == 0) {
            std::cout << std::endl;
        }
    }
}

TEST_F(WeeverTaxBenchmark, BatchDelete) {
    std::printf("\n# Tax batch delete benchmark\n");
    std::fflush(stdout);

    for (size_t b = 0; b < num_batches; ++b) {
        std::unordered_set<size_t> del_ids;
        del_ids.reserve(batch_size);
        size_t const first = index_offset + b * batch_size;
        for (size_t i = 0; i < batch_size; ++i) del_ids.insert(first + i);

        weever->SetOption(kDeleteStatements, std::move(del_ids));
        weever->SetOption(kInsertStatements, {});
        weever->SetOption(kUpdateStatements, {});

        auto t0 = std::chrono::steady_clock::now();
        weever->Execute();
        auto mcs = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - t0);

        size_t vc = weever->GetViolationsSize();
        if ((b + 1) % 5 == 0) {
            auto mcs_str = std::format("{:.3g}", (double)mcs.count() / 1000);
            std::printf("(%zu, %s) ", b + 1, mcs_str.c_str());
            // std::printf("vc: %ld", vc);
        }
        if ((b + 1) % 25 == 0) {
            std::cout << std::endl;
        }
    }
}

TEST_F(WeeverTaxBenchmark, BatchUpdate) {
    std::printf("\n# Tax batch update benchmark\n");
    std::fflush(stdout);

    for (size_t b = 0; b < num_batches; ++b) {
        std::vector<model::IDatasetStream::Row> upd_rows;
        upd_rows.reserve(batch_size);
        for (size_t i = 0; i < batch_size; ++i) {
            size_t const tax_idx = b * batch_size + i;
            model::IDatasetStream::Row src = tax_rows[tax_idx];
            src.insert(src.begin(), std::to_string(index_offset + tax_idx));
            upd_rows.push_back(std::move(src));
        }
        auto upd_tbl = std::make_shared<MockTable>("epic_upd", tax_cols, std::move(upd_rows));

        weever->SetOption(kInsertStatements, config::InputTable{});
        weever->SetOption(kDeleteStatements, std::unordered_set<size_t>{});
        weever->SetOption(kUpdateStatements, config::InputTable(std::move(upd_tbl)));

        auto t0 = std::chrono::steady_clock::now();
        weever->Execute();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0);

        if ((b + 1) % 5 == 0) {
            std::printf("(%zu, %ld) ", b + 1, ms.count());
        }
        if ((b + 1) % 25 == 0) {
            std::cout << std::endl;
            // std::printf("vc: %ld\n", weever->GetViolationsSize());
        }
    }
}

TEST(WeeverBatchSizeBenchmark, TaxBatchComparison) {
    static constexpr size_t kBatchSize = 100;
    static constexpr size_t kBatchesPerMeasurement = 10;
    static constexpr size_t kNumMeasurements = 1000;

    std::string const dc = "!(s.State == t.State and s.Salary > t.Salary and s.Rate < t.Rate)";
    std::vector<std::string> const tax_cols = {
            "FName",  "LName", "Gender",      "AreaCode",      "Phone",
            "City",   "State", "Zip",         "MaritalStatus", "HasChild",
            "Salary", "Rate",  "SingleExemp", "MarriedExemp",  "ChildExemp"};

    std::vector<model::IDatasetStream::Row> all_tax_rows;
    {
        auto parser = std::make_shared<CSVParser>(kTax);
        while (parser->HasNextRow()) all_tax_rows.push_back(parser->GetNextRow());
    }

    std::mt19937_64 rng(42);
    std::uniform_int_distribution<size_t> dist(0, all_tax_rows.size() - 1);

    auto empty_table = std::make_shared<MockTable>("tax_empty", tax_cols,
                                                   std::vector<model::IDatasetStream::Row>{});
    algos::StdParamsMap params = {
            {kTable, config::InputTable(empty_table)},
            {kDenialConstraint, dc},
    };
    auto weever = algos::CreateAndLoadAlgorithm<Weever>(params);
    // weever->Execute();
    std::fprintf(stderr, "Starting inserts into empty table\n");

    std::printf("\\addplot coordinates {\n");

    size_t total_rows = 0;
    for (size_t m = 0; m < kNumMeasurements; ++m) {
        auto t0 = std::chrono::steady_clock::now();

        for (size_t b = 0; b < kBatchesPerMeasurement; ++b) {
            std::vector<model::IDatasetStream::Row> batch;
            batch.reserve(kBatchSize);
            for (size_t i = 0; i < kBatchSize; ++i) {
                batch.push_back(all_tax_rows[dist(rng)]);
            }
            auto ins_tbl = std::make_shared<MockTable>("ins", tax_cols, std::move(batch));

            weever->SetOption(kInsertStatements, config::InputTable(std::move(ins_tbl)));
            weever->SetOption(kDeleteStatements, std::unordered_set<size_t>{});
            weever->SetOption(kUpdateStatements, config::InputTable{});
            weever->Execute();
        }

        long long const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now() - t0)
                                     .count();

        total_rows += kBatchesPerMeasurement * kBatchSize;
        std::printf("(%zu, %lld) ", total_rows, ms);
        if ((m + 1) % 5 == 0) std::printf("\n");
    }

    std::printf("};\n");
    std::fflush(stdout);
}

}  // namespace tests
