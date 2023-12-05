#include <filesystem>
#include <memory>

#include <gtest/gtest.h>

#include "algorithms/cfd/model/cfd_relation_data.h"
#include "parser/csv_parser/csv_parser.h"
#include "table_config.h"

namespace tests {

namespace fs = std::filesystem;
namespace mo = model;

class TestCFDRelationData : public ::testing::Test {};

static fs::path ConstructPath(std::string_view dataset) {
    return test_data_dir / "cfd_data" / dataset;
}

TEST(TestCFDRelationData, TennisDataSet) {
    std::string_view tennis_path = "tennis.csv";
    CSVParser parser{ConstructPath(tennis_path), ',', true};
    std::shared_ptr<algos::cfd::CFDRelationData> relation_ =
            algos::cfd::CFDRelationData::CreateFrom(parser, 0, 0, 1, 1);

    std::string tennis_string =
            "outlook temp humidity windy play\n"
            "sunny hot high false no\n"
            "sunny hot high true no\n"
            "overcast hot high false yes\n"
            "rainy mild high false yes\n"
            "rainy cool normal false yes\n"
            "rainy cool normal true no\n"
            "overcast cool normal true yes\n"
            "sunny mild high false no\n"
            "sunny cool normal false yes\n"
            "rainy mild normal false yes\n"
            "sunny mild normal true yes\n"
            "overcast mild high true yes\n"
            "overcast hot normal false yes\n"
            "rainy mild high true no\n";

    std::string tennis_partial_string =
            "outlook temp humidity windy play\n"
            "sunny hot high true no\n"
            "rainy mild high false yes\n"
            "rainy cool normal true no\n"
            "overcast cool normal true yes\n";
    std::vector<int> my_tidlist = {1, 3, 5, 6};
    ASSERT_EQ(relation_->GetNumRows(), 14);
    ASSERT_EQ(relation_->GetStringFormat(), tennis_string);
    ASSERT_EQ(relation_->GetStringFormat(my_tidlist), tennis_partial_string);

    CSVParser new_parser{ConstructPath(tennis_path), ',', true};
    std::shared_ptr<algos::cfd::CFDRelationData> new_relation_ =
            algos::cfd::CFDRelationData::CreateFrom(new_parser, 3, 4, 1, 1);

    tennis_string =
            "outlook temp humidity\n"
            "sunny hot high\n"
            "sunny hot high\n"
            "overcast hot high\n"
            "rainy mild high\n";
    ASSERT_EQ(new_relation_->GetNumRows(), 4);
    ASSERT_EQ(new_relation_->GetStringFormat(), tennis_string);
}

}  // namespace tests
