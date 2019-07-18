 //
// Created by kek on 17.07.19.
//

#include "Vertical.h"
#include "RelationalSchema.h"
#include <utility>

using namespace std;

Vertical::Vertical(RelationalSchema *relSchema, int indices) :
    schema(relSchema),
    columnIndices(indices) {}

Vertical::Vertical(Vertical &&other) noexcept :
    schema(other.schema),
    columnIndices(std::move(other.columnIndices)) {
    other.schema = nullptr;
}

Vertical& Vertical::operator=(Vertical &&rhs) noexcept {
    columnIndices = std::move(rhs.columnIndices);
    schema = rhs.schema;
    rhs.schema = nullptr;
}

dynamic_bitset<>* Vertical::getColumnIndices() { return &columnIndices; }

RelationalSchema* Vertical::getSchema() { return schema; }

//TODO: перепроверь цикл
//TODO: перепроверь все операции с Джавой
bool Vertical::contains(Vertical &that) {
    dynamic_bitset<>& thisIndices = columnIndices;
    dynamic_bitset<>& thatIndices = that.columnIndices;
    if(thisIndices.count() < thatIndices.count()) return false;
    for (int columnIndex = thatIndices.find_first(); columnIndex < thatIndices.size(); columnIndex = thatIndices.find_next(columnIndex + 1)){
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
    retainedColumnIndices |= *(that.getColumnIndices());
    schema->getVertical(retainedColumnIndices);
}

Vertical Vertical::project(Vertical &that) {
    dynamic_bitset<> retainedColumnIndices(columnIndices);
    retainedColumnIndices &= that.columnIndices;
    return schema->getVertical(retainedColumnIndices);
}

Vertical Vertical::without(Vertical &that) {
    dynamic_bitset<> retainedColumnIndices(columnIndices);
    retainedColumnIndices &= that.columnIndices;
    retainedColumnIndices = ~retainedColumnIndices;
    return schema->getVertical(retainedColumnIndices);
}

Vertical Vertical::invert() {
    dynamic_bitset<> flippedIndices(columnIndices);
    flippedIndices.flip();
    return schema->getVertical(flippedIndices);
}

Vertical Vertical::invert(Vertical &scope) {
    dynamic_bitset<> flippedIndices(columnIndices);
    flippedIndices ^= scope.columnIndices;
    return schema->getVertical(flippedIndices);
}

Vertical Vertical::emptyVertical(RelationalSchema *relSchema) {
    return Vertical(relSchema, 0);
}

int Vertical::getArity() {
    return columnIndices.count();
}

string Vertical::toString() { return "[]";}