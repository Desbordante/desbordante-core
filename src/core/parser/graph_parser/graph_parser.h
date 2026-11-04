#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/algorithms/gfd/gfd.h"
#include "core/algorithms/gfd/graph_descriptor.h"

namespace parser::graph_parser {

namespace gfd {

model::gfd::graph_t ReadGraph(std::istream& stream);
model::gfd::graph_t ReadGraph(std::filesystem::path const& path);

void WriteGraph(std::ostream& stream, model::gfd::graph_t const& result);
void WriteGraph(std::filesystem::path const& path, model::gfd::graph_t const& result);

model::Gfd ReadGfd(std::istream& stream);
model::Gfd ReadGfd(std::filesystem::path const& path);

void WriteGfd(std::ostream& stream, model::Gfd const& result);
void WriteGfd(std::filesystem::path const& path, model::Gfd const& result);

}  // namespace gfd

namespace gdd {

model::gdd::GraphTypes::Graph ReadGraph(std::istream& stream);
model::gdd::GraphTypes::Graph ReadGraph(std::filesystem::path const& path);

}  // namespace gdd

}  // namespace parser::graph_parser