//
// Created by kek on 16.08.2019.
//

#pragma once

#include "Vertical.h"
#include <boost/dynamic_bitset.hpp>

using boost::dynamic_bitset, std::string;

class ColumnCombination : public Vertical {

public:
    ColumnCombination(dynamic_bitset<> columnIndices, shared_ptr<RelationalSchema> schema);
    string toString() override ;
};
