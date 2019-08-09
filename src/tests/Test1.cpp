//
// Created by kek on 07.08.2019.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../model/ColumnLayoutRelationData.h"

using ::testing::ContainerEq, ::testing::Eq;

TEST(first, kek){
    vector<vector<int>> ans = {
            {0, 2, 8, 11},
            {1, 5, 9},
            {4, 14},
            {6, 7},
            {10, 17} //null
    };

    CSVParser csvParser("Test1.csv");
    auto test = ColumnLayoutRelationData::createFrom(csvParser, true);
    for (auto & columnData : test->getColumnData()) {
        auto index = columnData->getPositionListIndex()->getIndex();
        ASSERT_THAT(ans, ContainerEq(index));
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}