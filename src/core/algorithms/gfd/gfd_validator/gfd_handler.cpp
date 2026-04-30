#include "core/algorithms/gfd/gfd_validator/gfd_handler.h"

#include <iostream>
#include <set>
#include <thread>

#include <boost/graph/eccentricity.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/vf2_sub_graph_iso.hpp>

#include "core/config/equal_nulls/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/thread_number/option.h"
#include "core/util/logger.h"

namespace algos {

GfdHandler::GfdHandler() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::names::kGfdData, config::names::kGraphData});
};

void GfdHandler::RegisterOptions() {
    using namespace config::names;
    using namespace config::descriptions;
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::Option{&gfd_paths_, kGfdData, kDGfdData});
    RegisterOption(config::Option{&graph_path_, kGraphData, kDGraphData});
}

void GfdHandler::LoadDataInternal() {
    std::ifstream f(graph_path_);
    graph_ = parser::graph_parser::ReadGraph(f);
    f.close();
    for (auto const& path : gfd_paths_) {
        auto gfd_path = path;
        f.open(gfd_path);
        model::Gfd gfd = parser::graph_parser::ReadGfd(f);
        f.close();
        gfds_.push_back(gfd);
    }
}

void GfdHandler::ResetState() {}

void GfdHandler::ExecuteInternal() {
    result_ = GenerateSatisfiedGfds(graph_, gfds_);

    LOG_DEBUG("Satisfied GFDs: {}/{}", result_.size(), gfds_.size());
}

}  // namespace algos
