//
// Created by kek on 18.07.19.
//

#pragma once

#include <string>

#include <boost/dynamic_bitset.hpp>

#include "model/Vertical.h"

using std::string, boost::dynamic_bitset;

class RelationalSchema;

class Column : public Vertical {
friend RelationalSchema;

protected:
    string name;
    int index;

public:
    Column(shared_ptr<RelationalSchema> schema, string name, int index);
    Column(shared_ptr<RelationalSchema> schema, string name, int index, int size);       //for TANE implementation (needed: each two vertices have equal bitset sizes
    int getIndex();
    string getName();
    string toString() override;
    bool operator==(const Column& rhs);
};