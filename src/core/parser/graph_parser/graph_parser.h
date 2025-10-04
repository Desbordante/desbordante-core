#pragma once
#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <string>
#include <vector>

#include "algorithms/gfd/gfd.h"
#include "algorithms/gfd/graph_descriptor.h"

namespace parser {

namespace graph_parser {

model::graph_t ReadGraph(std::istream& stream);
model::graph_t ReadGraph(std::filesystem::path const& path);

void WriteGraph(std::ostream& stream, model::graph_t const& result);
void WriteGraph(std::filesystem::path const& path, model::graph_t const& result);

model::Gfd ReadGfd(std::istream& stream);
model::Gfd ReadGfd(std::filesystem::path const& path);

void WriteGfd(std::ostream& stream, model::Gfd const& result);
void WriteGfd(std::filesystem::path const& path, model::Gfd const& result);

}  // namespace graph_parser

}  // namespace parser
