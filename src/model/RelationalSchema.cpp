//
// Created by kek on 17.07.19.
//

#include "RelationalSchema.h"
#include <utility>

using namespace std;

RelationalSchema::RelationalSchema(string name, bool isNullEqNull) :
        columns(),
        name(std::move(name)),
        isNullEqNull(isNullEqNull),
        emptyVertical() {
}

shared_ptr<RelationalSchema> RelationalSchema::create(string name, bool isNullEqNull) {
    auto schema = shared_ptr<RelationalSchema>(new RelationalSchema(std::move(name), isNullEqNull));
    schema->init();
    return schema;
}

void RelationalSchema::init() {
    emptyVertical.reset(new Vertical(std::move(Vertical::emptyVertical(shared_from_this()))));
}

//TODO: Перепроверь
Vertical RelationalSchema::getVertical(dynamic_bitset<> indices) {
    return *emptyVertical;
}

string RelationalSchema::getName() { return name; }

vector<shared_ptr<Column>> RelationalSchema::getColumns() { return columns; }

//TODO: assert'ы пофиксить на нормальные эксепшены
shared_ptr<Column> RelationalSchema::getColumn(const string &colName) {
    for (auto &column : columns){
        if (column->name == colName)
            return column;
    }
    assert(0);
}

shared_ptr<Column> RelationalSchema::getColumn(int index) {
    return columns[index];
}

void RelationalSchema::appendColumn(const string& colName) {
    columns.push_back(make_shared<Column>(shared_from_this(), colName, columns.size()));
}

int RelationalSchema::getNumColumns() {
    return columns.size();
}

bool RelationalSchema::isNullEqualNull() { return isNullEqNull; }

