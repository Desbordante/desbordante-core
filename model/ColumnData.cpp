//
// Created by kek on 12.07.19.
//

#include "ColumnData.h"
#include <utility>
#include <algorithm>
#include <random>

using namespace std;

ColumnData::ColumnData(Column *column, vector<int> probingTable, PositionListIndex* positionListIndex):
    column(column),
    probingTable(std::move(probingTable)),
    positionListIndex(positionListIndex)
    {}

vector<int> ColumnData::getProbingTable() { return probingTable; }

int ColumnData::getProbingTableValue(int tupleIndex) { return probingTable[tupleIndex]; }

Column* ColumnData::getColumn() { return column; }

PositionListIndex* ColumnData::getPositionListIndex() { return positionListIndex; }

void ColumnData::shuffle() {
    random_device rd;
    mt19937 random(rd());
    std::shuffle(probingTable.begin(), probingTable.end(), random);
}

string ColumnData::toString() { return "Data for " + column->toString(); }

bool ColumnData::operator==(const ColumnData &rhs) {
    if (this == &rhs) return true;
    return this->column == rhs.column;
}