#include <gtest/gtest.h>
#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/algorithms/rfd/similarity_metric.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "tests/common/all_csv_configs.h"

namespace algos::rfd {

TEST(GaRfdTest, BasicDiscovery) {
    GaRfd algo;
    config::InputTable table = std::make_shared<CSVParser>(tests::kIris);
    algo.SetOption(config::names::kTable, table);
    
    // Устанавливаем все параметры явно
    algo.SetOption("similarity_thresholds", std::vector<double>{0.8, 0.8, 0.8, 0.8, 0.8});
    algo.SetOption("min_confidence", 0.9);
    algo.SetOption("population_size", std::size_t{20});
    algo.SetOption("max_generations", std::size_t{5});
    algo.SetOption("crossover_prob", 0.85);
    algo.SetOption("mutation_prob", 0.3);
    algo.SetOption("max_lhs_arity", std::size_t{3});
    algo.SetOption("seed", std::uint64_t{42});
    algo.SetOption("output_file", std::string{});
    // Пустой вектор метрик — в LoadDataInternal подставятся Levenshtein
    algo.SetOption("metrics", std::vector<std::shared_ptr<SimilarityMetric>>{});
    
    algo.LoadData();
    algo.Execute();
    auto results = algo.GetResultStrings();
    EXPECT_FALSE(results.empty());
}

}  // namespace algos::rfd