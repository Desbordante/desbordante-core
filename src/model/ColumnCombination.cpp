//
// Created by kek on 16.08.2019.
//

#include "model/ColumnCombination.h"

#include "model/RelationalSchema.h"

ColumnCombination::ColumnCombination(dynamic_bitset<> columnIndices, shared_ptr<RelationalSchema> schema):
    Vertical(){
    this->columnIndices = std::move(columnIndices);
    this->schema = schema;
}

string ColumnCombination::toString() {
    string ans = "[";
    string separator;
    auto relation = schema.lock();
    for (unsigned long index = columnIndices.find_first(); index < columnIndices.size(); index = columnIndices.find_next(index + 1)){
        ans += separator + relation->getColumn(index)->getName();
        separator = ", ";
    }
    return ans;
}