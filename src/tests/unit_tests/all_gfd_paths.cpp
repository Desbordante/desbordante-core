#include "all_gfd_paths.h"

#include "csv_config_util.h"

namespace tests {

model::Gfd MakeGfd(std::filesystem::path const& gfd_path) {
    std::ifstream f(gfd_path);
    return parser::graph_parser::ReadGfd(f);
};

namespace {

std::filesystem::path const kCurrentPath = kTestDataDir / "graph_data";

}  // namespace

std::filesystem::path const kGfdTestBlogsGfd = kCurrentPath / "blogs_gfd.dot";
std::filesystem::path const kGfdTestBlogsGraph = kCurrentPath / "blogs_graph.dot";
std::filesystem::path const kGfdTestChannelsGfd = kCurrentPath / "channels_gfd.dot";
std::filesystem::path const kGfdTestChannelsGraph = kCurrentPath / "channels_graph.dot";
std::filesystem::path const kGfdTestMoviesGraph = kCurrentPath / "movies_graph.dot";
std::filesystem::path const kGfdTestSymbolsGfd1 = kCurrentPath / "symbols_gfd1.dot";
std::filesystem::path const kGfdTestSymbolsGfd2 = kCurrentPath / "symbols_gfd2.dot";
std::filesystem::path const kGfdTestSymbolsGraph = kCurrentPath / "symbols_graph.dot";
std::filesystem::path const kGfdTestShapesGfd1 = kCurrentPath / "shapes_gfd1.dot";
std::filesystem::path const kGfdTestShapesGfd2 = kCurrentPath / "shapes_gfd2.dot";
std::filesystem::path const kGfdTestShapesGraph = kCurrentPath / "shapes_graph.dot";
std::filesystem::path const kGfdTestQuadrangleGfd = kCurrentPath / "quadrangle_gfd.dot";
std::filesystem::path const kGfdTestQuadrangleGraph = kCurrentPath / "quadrangle.dot";
std::filesystem::path const kGfdTestDirectorsGfd = kCurrentPath / "directors_gfd.dot";
std::filesystem::path const kGfdTestDirectorsGraph = kCurrentPath / "directors.dot";
}  // namespace tests
