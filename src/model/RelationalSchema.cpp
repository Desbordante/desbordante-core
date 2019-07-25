//
// Created by kek on 17.07.19.
//

#include "RelationalSchema.h"
#include <utility>

using namespace std;

RelationalSchema::RelationalSchema(string name, bool isNullEqNull) :
    name(move(name)),
    emptyVertical(Vertical::emptyVertical(this)),
    isNullEqNull(isNullEqNull),
    columns() {}

Vertical RelationalSchema::getVertical(dynamic_bitset<> indices) {
    return emptyVertical;
}

string RelationalSchema::getName() { return name; }

vector<Column>& RelationalSchema::getColumns() { return columns; }

//TODO: assert'ы пофиксить на нормальные эксепшены
Column& RelationalSchema::getColumn(const string &colName) {
    for (auto &column : columns){
        if (column.name == colName)
            return column;
    }
    assert(0);
}

Column& RelationalSchema::getColumn(int index) {
    return columns[index];
}

void RelationalSchema::appendColumn(const string& colName) {
    columns.emplace_back(this, colName, columns.size());
}

int RelationalSchema::getNumColumns() {
    return columns.size();
}

bool RelationalSchema::isNullEqualNull() { return isNullEqNull; }

