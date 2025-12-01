#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/tane/pfdtane.h"
#include "core/config/names.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
namespace onam = config::names;

struct PFD {
    std::size_t lhs_id;
    std::size_t rhs_id;
    config::ErrorType expected_error;
};

struct PFDTaneMiningParams {
    algos::StdParamsMap params;
    unsigned int result_hash;

    PFDTaneMiningParams(unsigned int result_hash, config::ErrorType error,
                        algos::PfdErrorMeasure error_measure, CSVConfig const& csv_config)
        : params({{onam::kCsvConfig, csv_config},
                  {onam::kError, error},
                  {onam::kPfdErrorMeasure, error_measure}}),
          result_hash(result_hash) {}
};

struct PFDTaneValidationParams {
    std::vector<PFD> fds;
    algos::PfdErrorMeasure error_measure;
    CSVConfig csv_config;
};

class TestPFDTaneMining : public ::testing::TestWithParam<PFDTaneMiningParams> {};

class TestPFDTaneValidation : public ::testing::TestWithParam<PFDTaneValidationParams> {};

TEST_P(TestPFDTaneMining, DefaultTest) {
    auto const& p = GetParam();
    auto algos = algos::CreateAndLoadAlgorithm<algos::PFDTane>(p.params);
    algos->Execute();
    EXPECT_EQ(p.result_hash, algos->Fletcher16());
}

TEST_P(TestPFDTaneValidation, ErrorCalculationTest) {
    auto const& p = GetParam();
    double eps = 0.00001;
    auto table = std::make_shared<CSVParser>(p.csv_config);
    auto relation = ColumnLayoutRelationData::CreateFrom(*table, true);

    for (auto const& [lhs_id, rhs_id, expected_error] : p.fds) {
        auto const& lhs = relation->GetColumnData(lhs_id).GetPositionListIndex();
        auto const& rhs = relation->GetColumnData(rhs_id).GetPositionListIndex();
        config::ErrorType error =
                algos::PFDTane::CalculatePFDError(lhs, lhs->Intersect(rhs).get(), p.error_measure);
        EXPECT_NEAR(error, expected_error, eps);
    }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        PFDTaneTestMiningSuite, TestPFDTaneMining,
        ::testing::Values(
            PFDTaneMiningParams(44381, 0.3, +algos::PfdErrorMeasure::per_value, kTestFD),
            PFDTaneMiningParams(19266, 0.1, +algos::PfdErrorMeasure::per_value, kIris),
            PFDTaneMiningParams(10695, 0.01, +algos::PfdErrorMeasure::per_value, kIris),
            PFDTaneMiningParams(44088, 0.1, +algos::PfdErrorMeasure::per_value, kNeighbors10k),
            PFDTaneMiningParams(41837, 0.01, +algos::PfdErrorMeasure::per_value, kNeighbors10k)
        ));

INSTANTIATE_TEST_SUITE_P(
        PFDTaneTestValidationSuite, TestPFDTaneValidation,
        ::testing::Values(
                    PFDTaneValidationParams({{2, 3, 0.0625}, {4, 5, 0.333333}, {3, 2, 0.291666}, {0, 1, 0.75},
                                             {1, 0, 0.0}, {4, 3, 0.099999}, {1, 5, 0.416666}, {5, 1, 0.0}}, +algos::PfdErrorMeasure::per_value, kTestFD),
                    PFDTaneValidationParams({{2, 3, 0.083333}, {4, 5, 0.333333}, {3, 2, 0.5}, {0, 1, 0.75},
                                             {1, 0, 0.0}, {4, 3, 0.083333}, {1, 5, 0.416666}, {5, 1, 0.0}}, +algos::PfdErrorMeasure::per_tuple, kTestFD)
                ));

// clang-format on

}  // namespace tests
