#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "algorithms/gfd/gfd.h"
#include "algorithms/gfd/graph_descriptor.h"

namespace parser {

namespace graph_parser {

graph_t ReadGraph(std::istream& stream);
graph_t ReadGraph(std::filesystem::path const& path);

void WriteGraph(std::ostream& stream, graph_t& result);
void WriteGraph(std::filesystem::path const& path, graph_t& result);

Gfd ReadGfd(std::istream& stream);
Gfd ReadGfd(std::filesystem::path const& path);

void WriteGfd(std::ostream& stream, Gfd& result);
void WriteGfd(std::filesystem::path const& path, Gfd& result);

}  // namespace graph_parser

}  // namespace parser
