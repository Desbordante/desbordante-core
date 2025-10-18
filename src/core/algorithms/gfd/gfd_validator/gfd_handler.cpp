#include "algorithms/gfd/gfd_validator/gfd_handler.h"

#include <chrono>
#include <fstream>
#include <iosfwd>
#include <string_view>

#include <easylogging++.h>

#include "algorithm.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "gfd/gfd.h"
#include "graph_parser/graph_parser.h"
#include "option.h"

namespace algos {

GfdHandler::GfdHandler() : Algorithm({}) {
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

unsigned long long GfdHandler::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();

    result_ = GenerateSatisfiedGfds(graph_, gfds_);

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG(DEBUG) << "Satisfied GFDs: " << result_.size() << "/" << gfds_.size();
    return elapsed_milliseconds.count();
}

}  // namespace algos
