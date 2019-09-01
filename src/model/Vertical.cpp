//
// Created by kek on 17.07.19.
//

#include "Vertical.h"
#include "RelationalSchema.h"
#include <utility>

using namespace std;

Vertical::Vertical(shared_ptr<RelationalSchema>& relSchema, int indices) :
        columnIndices(indices),
        schema(relSchema) {}

Vertical::Vertical(Vertical &&other) noexcept :
    columnIndices(std::move(other.columnIndices)),
    schema(std::move(other.schema)) {
}

Vertical& Vertical::operator=(Vertical &&rhs) noexcept {
    columnIndices = std::move(rhs.columnIndices);
    schema = std::move(rhs.schema);
    return *this;
}

dynamic_bitset<> Vertical::getColumnIndices() { return columnIndices; }

shared_ptr<RelationalSchema> Vertical::getSchema() { return schema.lock(); }

//TODO: перепроверь цикл
//TODO: перепроверь все операции с Джавой
bool Vertical::contains(Vertical &that) {
    dynamic_bitset<>& thisIndices = columnIndices;
    dynamic_bitset<>& thatIndices = that.columnIndices;
    if(thisIndices.count() < thatIndices.count()) return false;
    for (unsigned long columnIndex = thatIndices.find_first(); columnIndex < thatIndices.size(); columnIndex = thatIndices.find_next(columnIndex + 1)){
        if (!(thisIndices[columnIndex])) return false;
    }
    return true;
}

bool Vertical::intersects(Vertical &that) {
    dynamic_bitset<>& thisIndices = columnIndices;
    dynamic_bitset<>& thatIndices = that.columnIndices;
    return thisIndices.intersects(thatIndices);
}

Vertical Vertical::Union(Vertical &that) {
    dynamic_bitset<> retainedColumnIndices(columnIndices);
    retainedColumnIndices |= that.columnIndices;
    return schema.lock()->getVertical(retainedColumnIndices);
}

Vertical Vertical::project(Vertical &that) {
    dynamic_bitset<> retainedColumnIndices(columnIndices);
    retainedColumnIndices &= that.columnIndices;
    return schema.lock()->getVertical(retainedColumnIndices);
}

Vertical Vertical::without(Vertical &that) {
    dynamic_bitset<> retainedColumnIndices(columnIndices);
    retainedColumnIndices &= that.columnIndices;
    retainedColumnIndices = ~retainedColumnIndices;
    return schema.lock()->getVertical(retainedColumnIndices);
}

Vertical Vertical::invert() {
    auto relation = schema.lock();
    dynamic_bitset<> flippedIndices(columnIndices);
    flippedIndices.resize(relation->getNumColumns());
    flippedIndices.flip(0, getSchema()->getNumColumns());
    return relation->getVertical(flippedIndices);
}

Vertical Vertical::invert(Vertical &scope) {
    dynamic_bitset<> flippedIndices(columnIndices);
    flippedIndices ^= scope.columnIndices;
    return schema.lock()->getVertical(flippedIndices);
}

Vertical Vertical::emptyVertical(shared_ptr<RelationalSchema> relSchema) {
    return Vertical(relSchema, 0);
}

int Vertical::getArity() {
    return columnIndices.count();
}

vector<shared_ptr<Column>> Vertical::getColumns() {
    //dynamic_bitset<> returnColumnIndices = getColumnIndices();
    auto relation = schema.lock();
    vector<shared_ptr<Column>> columns;
    for (int index = columnIndices.find_first();
         index != dynamic_bitset<>::npos;
         index = columnIndices.find_next(index + 1)) {
        columns.push_back(relation->getColumns()[index]);
    }
    return columns;
}

string Vertical::toString() { return "[]";}
