#pragma once
#include <string>
#include <vector>

#include "graph_descriptor.h"

namespace model {

class Gfd {
public:
    // Defines a specific attribute of the pattern.
    // The first element is the index of the vertex,
    // the second is the name of the attribute.
    // An alias for user convenience.
    using Token = std::pair<int, std::string>;

    // Concept from the article "Discovering Graph Functional Dependencies"
    // by Fan Wenfei, Hu Chunming, Liu Xueli, and Lu Ping.
    // An alias for user convenience.
    using Literal = std::pair<Token, Token>;

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

    std::string ToString();
};

}  // namespace model
