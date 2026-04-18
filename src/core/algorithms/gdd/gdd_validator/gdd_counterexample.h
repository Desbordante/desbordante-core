#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace algos {

struct GddCounterexampleVertex {
    std::size_t pattern_vertex_id;
    std::string pattern_vertex_label;

    std::size_t graph_vertex_id;
    std::string graph_vertex_label;
    std::unordered_map<std::string, std::string> graph_vertex_attributes;
};

struct GddCounterexample {
    std::size_t gdd_index;
    std::vector<GddCounterexampleVertex> match;
};

}  // namespace algos
