#pragma once

#include <boost/dynamic_bitset.hpp>
#include <boost/lexical_cast.hpp>

#include "core/model/index.h"

class PartialFD {
public:
    double error_;
    boost::dynamic_bitset<> lhs_;
    model::Index rhs_;
    double score_;

    PartialFD(boost::dynamic_bitset<> lhs, model::Index rhs, double error, double score)
        : error_(error), lhs_(std::move(lhs)), rhs_(std::move(rhs)), score_(score) {}

    double GetError() const {
        return error_;
    }

    int GetArity() const {
        return lhs_.count();
    }
};
