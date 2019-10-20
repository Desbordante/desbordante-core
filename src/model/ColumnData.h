//
// Created by kek on 12.07.19.
//

#pragma once

#include <vector>

#include "model/Column.h"
#include "util/PositionListIndex.h"

using std::vector;

class ColumnData {
private:
    shared_ptr<Column> column;
    vector<int> probingTable;
    shared_ptr<PositionListIndex> positionListIndex;

public:
    ColumnData(shared_ptr<Column>& column, vector<int> probingTable, shared_ptr<PositionListIndex>& positionListIndex);
    vector<int> getProbingTable();
    int getProbingTableValue(int tupleIndex);
    shared_ptr<Column> getColumn();
    shared_ptr<PositionListIndex> getPositionListIndex();
    void shuffle();
    string toString();
    bool operator==(const ColumnData& rhs);
};