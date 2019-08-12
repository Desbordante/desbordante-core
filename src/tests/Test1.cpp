//
// Created by kek on 07.08.2019.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../model/ColumnLayoutRelationData.h"
#include <iostream>

using ::testing::ContainerEq, ::testing::Eq;
using namespace std;

std::string get_selfpath();

TEST(pliChecker, first){
    vector<vector<int>> ans = {
            {0, 2, 8, 11},
            {1, 5, 9},
            {4, 14},
            {6, 7, 18},
            {10, 17} //null
    };

    auto path = fs::path(get_selfpath()).parent_path().string();
    CSVParser csvParser(path + "/Test1.csv");
    auto test = ColumnLayoutRelationData::createFrom(csvParser, true);
    for (auto & columnData : test->getColumnData()) {
        auto index = columnData->getPositionListIndex()->getIndex();
        ASSERT_THAT(ans, ContainerEq(index));
    }
}

TEST(pliChecker, second){
    vector<vector<int>> ans = {
            {0, 2, 8, 11},
            {1, 5, 9},
            {4, 14},
            {6, 7, 18},
    };

    auto path = fs::path(get_selfpath()).parent_path().string();
    CSVParser csvParser(path + "/Test1.csv");
    auto test = ColumnLayoutRelationData::createFrom(csvParser, false);
    for (auto & columnData : test->getColumnData()) {
        auto index = columnData->getPositionListIndex()->getIndex();
        ASSERT_THAT(ans, ContainerEq(index));
    }
}