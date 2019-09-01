//
// Created by kek on 17.07.19.
//

#pragma once

#include <boost/dynamic_bitset.hpp>
#include <string>
#include <vector>
#include "Vertical.h"
#include "Column.h"
#include <memory>

using boost::dynamic_bitset, std::string, std::vector;

class RelationalSchema : public std::enable_shared_from_this<RelationalSchema> {
private:
    RelationalSchema(string name, bool isNullEqNull);
    void init();
    vector<shared_ptr<Column>> columns;
    string name;
    bool isNullEqNull;

public:
    shared_ptr<Vertical> emptyVertical;

    static shared_ptr<RelationalSchema> create(string name, bool isNullEqNull);
    string getName();
    vector<shared_ptr<Column>> getColumns();
    shared_ptr<Column> getColumn(const string &colName);
    shared_ptr<Column> getColumn(int index);
    void appendColumn(const string& colName);
    void appendColumn(shared_ptr<Column> column);
    int getNumColumns();
    Vertical getVertical(dynamic_bitset<> indices);
    bool isNullEqualNull();
};
