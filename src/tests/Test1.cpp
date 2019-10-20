//
// Created by kek on 07.08.2019.
//

#include "model/ColumnLayoutRelationData.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

    string path = "/home/maxim/Study/Metanome-coding/metanome_git/src/tests";//fs::path(get_selfpath()).parent_path().string();
    cout << path << endl;
    CSVParser csvParser(path + "/Test1.csv");
    auto test = ColumnLayoutRelationData::createFrom(csvParser, true);
    auto columnData = test->getColumnData(0);
    auto index = columnData->getPositionListIndex()->getIndex();
    ASSERT_THAT(ans, ContainerEq(index));
}

TEST(pliChecker, second){
    vector<vector<int>> ans = {
            {0, 2, 8, 11},
            {1, 5, 9},
            {4, 14},
            {6, 7, 18},
    };

    string path = "/home/maxim/Study/Metanome-coding/metanome_git/src/tests";//fs::path(get_selfpath()).parent_path().string();
    CSVParser csvParser(path + "/Test1.csv");
    auto test = ColumnLayoutRelationData::createFrom(csvParser, false);
    auto columnData = test->getColumnData(0);
    auto index = columnData->getPositionListIndex()->getIndex();
    ASSERT_THAT(ans, ContainerEq(index));
}