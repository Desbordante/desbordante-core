//
// Created by kek on 17.07.19.
//

#pragma once

#include <boost/dynamic_bitset.hpp>
#include <string>
#include <vector>
#include "Vertical.h"
#include "Column.h"

using boost::dynamic_bitset, std::string, std::vector;

class RelationalSchema {
public:
    string name;
    Vertical emptyVertical;
    bool isNullEqNull;
    vector<Column> columns;

    RelationalSchema(string name, bool isNullEqNull);
    string getName();
    vector<Column>& getColumns();
    Column& getColumn(const string &colName);
    Column& getColumn(int index);
    void appendColumn(const string& colName);
    int getNumColumns();
    //TODO: getVertical работает неверно, нужно будет подумать в зависимотси от его использования
    Vertical getVertical(dynamic_bitset<> indices);
    bool isNullEqualNull();
};
