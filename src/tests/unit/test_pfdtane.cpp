#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/tane/tane.h"
#include "core/config/names.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
namespace onam = config::names;

struct PFDTaneMiningParams {
    algos::StdParamsMap params;
    unsigned int result_hash;

    PFDTaneMiningParams(unsigned int result_hash, config::ErrorType error,
                        model::AfdMeasure measure, CSVConfig const& csv_config)
        : params({{onam::kCsvConfig, csv_config},
                  {onam::kError, error},
                  {onam::kAfdMeasure, measure}}),
          result_hash(result_hash) {}
};

class TestPFDTaneMining : public ::testing::TestWithParam<PFDTaneMiningParams> {};

TEST_P(TestPFDTaneMining, DefaultTest) {
    auto const& p = GetParam();
    auto algos = algos::CreateAndLoadAlgorithm<algos::Tane>(p.params);
    algos->Execute();
    EXPECT_EQ(p.result_hash, algos->Fletcher16());
}

INSTANTIATE_TEST_SUITE_P(
        PFDTaneTestMiningSuite, TestPFDTaneMining,
        ::testing::Values(
                PFDTaneMiningParams(44381, 0.3, model::AfdMeasure::kPerValue, kTestFD),
                PFDTaneMiningParams(19266, 0.1, model::AfdMeasure::kPerValue, kIris),
                PFDTaneMiningParams(10695, 0.01, model::AfdMeasure::kPerValue, kIris),
                PFDTaneMiningParams(44088, 0.1, model::AfdMeasure::kPerValue, kNeighbors10k),
                PFDTaneMiningParams(41837, 0.01, model::AfdMeasure::kPerValue, kNeighbors10k)));
}  // namespace tests
