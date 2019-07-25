//
// Created by kek on 12.07.19.
//

#pragma once

#include <vector>
#include "Column.h"

using std::vector;

class PositionListIndex {}; //TODO: Волшебный класс

class ColumnData {
private:
    Column* column;
    vector<int> probingTable;
    PositionListIndex* positionListIndex;

public:
    ColumnData(Column* column, vector<int> probingTable, PositionListIndex* positionListIndex);
    vector<int> getProbingTable();
    int getProbingTableValue(int tupleIndex);
    Column* getColumn();
    PositionListIndex* getPositionListIndex();
    void shuffle();
    string toString();
    bool operator==(const ColumnData& rhs);
};