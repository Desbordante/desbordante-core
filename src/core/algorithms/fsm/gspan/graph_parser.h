#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "frequent_subgraph.h"
#include "graph.h"

namespace gspan::parser {

std::vector<gspan::graph_t> ReadGraphs(std::istream& stream);
std::vector<gspan::graph_t> ReadGraphs(std::filesystem::path const& path);

void WriteGraphs(std::ostream& stream, std::vector<gspan::FrequentSubgraph> const& result);
void WriteGraphs(std::filesystem::path const& path,
                 std::vector<gspan::FrequentSubgraph> const& result);

}  // namespace gspan::parser