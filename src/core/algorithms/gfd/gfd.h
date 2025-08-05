#pragma once
#include <string>
#include <utility>
#include <vector>

#include "algorithms/gfd/graph_descriptor.h"

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

    template <typename T1, typename T2>
    Gfd(T1&& pattern, T2&& premises, T2&& conclusion)
        : pattern_(std::forward<T1>(pattern)),
          premises_(std::forward<T2>(premises)),
          conclusion_(std::forward<T2>(conclusion)) {}

    graph_t const& GetPattern() const noexcept {
        return pattern_;
    }

    std::vector<Literal> const& GetPremises() const noexcept {
        return premises_;
    }

    std::vector<Literal> const& GetConclusion() const noexcept {
        return conclusion_;
    }

    void SetPattern(graph_t const& pattern) {
        pattern_ = pattern;
    }

    void SetPremises(std::vector<Literal> const& premises) {
        premises_ = premises;
    }

    void SetConclusion(std::vector<Literal> const& conclusion) {
        conclusion_ = conclusion;
    }

    bool operator==(Gfd const& gfd) const;
    bool operator!=(Gfd const& gfd) const = default;

    std::string ToString() const;
};

}  // namespace model
