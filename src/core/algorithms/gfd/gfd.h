#pragma once
#include <string>
#include <vector>

#include "graph_descriptor.h"

using Token = std::pair<int, std::string>;
using Literal = std::pair<Token, Token>;

class Gfd {
private:
    graph_t pattern_;
    std::vector<Literal> premises_;
    std::vector<Literal> conclusion_;

public:
    Gfd() = default;

    Gfd(graph_t& pattern, std::vector<Literal>& premises, std::vector<Literal>& conclusion)
        : pattern_(pattern), premises_(premises), conclusion_(conclusion) {}

    graph_t GetPattern() const {
        return pattern_;
    }

    std::vector<Literal> GetPremises() const {
        return premises_;
    }

    std::vector<Literal> GetConclusion() const {
        return conclusion_;
    }

    void SetPattern(graph_t& pattern) {
        pattern_ = pattern;
    }

    void SetPremises(std::vector<Literal>& premises) {
        premises_ = premises;
    }

    void SetConclusion(std::vector<Literal>& conclusion) {
        conclusion_ = conclusion;
    }
};
