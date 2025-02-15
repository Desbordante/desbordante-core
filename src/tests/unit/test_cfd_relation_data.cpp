#include <memory>

#include <gtest/gtest.h>

#include "algorithms/cfd/model/cfd_relation_data.h"
#include "all_csv_configs.h"
#include "csv_config_util.h"

namespace tests {

class TestCFDRelationData : public ::testing::Test {};

TEST(TestCFDRelationData, TennisDataSet) {
    auto input_table = MakeInputTable(kTennis);
    std::shared_ptr<algos::cfd::CFDRelationData> relation =
            algos::cfd::CFDRelationData::CreateFrom(*input_table, 0, 0, 1, 1);

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
    ASSERT_EQ(relation->GetNumRows(), 14);
    ASSERT_EQ(relation->GetStringFormat(), tennis_string);
    ASSERT_EQ(relation->GetStringFormat(my_tidlist), tennis_partial_string);

    auto new_input_table = MakeInputTable(kTennis);
    std::shared_ptr<algos::cfd::CFDRelationData> new_relation =
            algos::cfd::CFDRelationData::CreateFrom(*new_input_table, 3, 4, 1, 1);

    tennis_string =
            "outlook temp humidity\n"
            "sunny hot high\n"
            "sunny hot high\n"
            "overcast hot high\n"
            "rainy mild high\n";
    ASSERT_EQ(new_relation->GetNumRows(), 4);
    ASSERT_EQ(new_relation->GetStringFormat(), tennis_string);
}

}  // namespace tests
