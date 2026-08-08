#pragma once

#include <boost/dynamic_bitset.hpp>
#include <boost/lexical_cast.hpp>

class PartialKey {
public:
    double error_;
    boost::dynamic_bitset<> vertical_;
    double score_;

    PartialKey(boost::dynamic_bitset<> vertical, double error, double score)
        : error_(error), vertical_(std::move(vertical)), score_(score) {}
};
