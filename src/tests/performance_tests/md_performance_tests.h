#pragma once

#include <memory>

#include "algorithms/algo_factory.h"
#include "algorithms/dd/split/split.h"
#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/hymd/hymd.h"
#include "algorithms/md/hymd/preprocessing/column_matches/levenshtein.h"
#include "all_csv_configs.h"
#include "config/names.h"
#include "config/thread_number/type.h"
#include "performance_testing.h"

namespace perf_tests {

inline void MDPerfTests() {
    using namespace config::names;
    using namespace algos::hymd;
    using preprocessing::column_matches::Levenshtein;

    auto& testing = PerformanceTesting::Instance();

    auto test = [] {
        constexpr static model::md::DecisionBoundary kMinSimilarity = 0.7;

        config::InputTable table = std::make_unique<CSVParser>(tests::kCIPublicHighway20attr60k);

        HyMD::ColumnMatches column_matches_option;
        std::size_t const number_of_columns = table->GetNumberOfColumns();
        column_matches_option.reserve(number_of_columns);
        for (size_t i = 0; i != number_of_columns; ++i) {
            column_matches_option.push_back(std::make_shared<Levenshtein>(i, i, kMinSimilarity));
        }

        algos::StdParamsMap param_map{
                {kLeftTable, table},
                {kThreads, static_cast<config::ThreadNumType>(1)},
                {kColumnMatches, column_matches_option},
        };
        auto algo = algos::CreateAndLoadAlgorithm<HyMD>(param_map);

        algo->Execute();
    };

    testing.RegisterTest(std::move(test), "HyMD, CIPublicHighway20attr60k");
}

}  // namespace perf_tests
