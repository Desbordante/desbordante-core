#pragma once

#include <filesystem>
#include <fstream>

#include "core/algorithms/gfd/gfd.h"
#include "core/parser/graph_parser/graph_parser.h"

namespace tests {

model::Gfd MakeGfd(std::filesystem::path const& gfd_path);

extern std::filesystem::path const kGfdTestBlogsGfd;
extern std::filesystem::path const kGfdTestBlogsGraph;
extern std::filesystem::path const kGfdTestChannelsGfd;
extern std::filesystem::path const kGfdTestChannelsGraph;
extern std::filesystem::path const kGfdTestMoviesGraph;
extern std::filesystem::path const kGfdTestSymbolsGfd1;
extern std::filesystem::path const kGfdTestSymbolsGfd2;
extern std::filesystem::path const kGfdTestSymbolsGraph;
extern std::filesystem::path const kGfdTestShapesGfd1;
extern std::filesystem::path const kGfdTestShapesGfd2;
extern std::filesystem::path const kGfdTestShapesGraph;
extern std::filesystem::path const kGfdTestQuadrangleGfd;
extern std::filesystem::path const kGfdTestQuadrangleGraph;
extern std::filesystem::path const kGfdTestDirectorsGfd;
extern std::filesystem::path const kGfdTestDirectorsGraph;
}  // namespace tests
