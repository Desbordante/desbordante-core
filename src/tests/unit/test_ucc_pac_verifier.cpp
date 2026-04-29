#include <memory>
#include <utility>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/config/custom_metric/custom_vector_metric.h"
#include "core/config/indices/type.h"
#include "core/config/names.h"

namespace tests {
using namespace config::names;
using namespace pac::model;

constexpr static auto kThreshold = 1e-3;

struct UCCPACVerifyingParams {
    algos::StdParamsMap params;
    double exp_epsilon;
    double exp_delta;

    UCCPACVerifyingParams(CSVConfig const& csv_config, config::IndicesType&& column_indices,
                          double expected_epsilon, double expected_delta, double max_delta = -1,
                          double min_epsilon = -1, double max_epsilon = -1,
                          unsigned long delta_steps = 0, bool dist_from_null_is_infinity = false,
                          std::shared_ptr<config::ICustomVectorMetric>&& metric = nullptr,
                          double diagonal_threshold = 1e-5)
        : params({
                  {kCsvConfig, csv_config},
                  {kColumnIndices, std::move(column_indices)},
                  {kMaxDelta, max_delta},
                  {kMinEpsilon, min_epsilon},
                  {kMaxEpsilon, max_epsilon},
                  {kDeltaSteps, delta_steps},
                  {kDistFromNullIsInfinity, dist_from_null_is_infinity},
                  {kMetric, std::move(metric)},
                  {kDiagonalThreshold, diagonal_threshold},
          }),
          exp_epsilon(expected_epsilon),
          exp_delta(expected_delta) {}
};

class TestUCCPACVerifier : public testing::TestWithParam<UCCPACVerifyingParams> {};
}  // namespace tests
