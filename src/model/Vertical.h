//
// Created by kek on 17.07.19.
//

#pragma once

#include <boost/dynamic_bitset.hpp>
#include <string>

using boost::dynamic_bitset, std::string;

class RelationalSchema;

class Vertical {
protected:
    Vertical(RelationalSchema* relSchema, int indices);

    dynamic_bitset<> columnIndices;
    RelationalSchema* schema;

public:
    Vertical(Vertical& other) = default;
    Vertical& operator=(const Vertical& rhs) = default;
    Vertical(Vertical&& other) noexcept;
    Vertical& operator=(Vertical&& rhs) noexcept ;

    dynamic_bitset<>* getColumnIndices();
    RelationalSchema* getSchema();
    bool contains(Vertical& that);
    bool intersects(Vertical& that);
    Vertical Union(Vertical& that);
    Vertical project(Vertical& that);
    Vertical without (Vertical& that);
    Vertical invert();
    Vertical invert(Vertical& scope);
    static Vertical emptyVertical(RelationalSchema* relSchema);
    int getArity();
    virtual string toString();
};
