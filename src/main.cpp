#include <iostream>
#include <stdexcept>

#include <boost/program_options.hpp>
#include <easylogging++.h>

#include "algorithms/algo_factory.h"
#include "algorithms/options/all_options.h"

INITIALIZE_EASYLOGGINGPP

namespace {
constexpr auto kHelp = "help";
constexpr auto kPrimitive = "primitive";
constexpr auto kDHelp = "print the help message and exit";

boost::program_options::options_description InfoOptions() {
    namespace po = boost::program_options;
    po::options_description info_options("Desbordante information options");
    info_options.add_options()(kHelp, kDHelp)
            // --version, if needed, goes here too
            ;
    return info_options;
}
}  // namespace

int main(int argc, char const* argv[]) {
    namespace po = boost::program_options;
    using namespace algos::config;

    std::string primitive;
    std::string const prim_desc = "algorithm to use for data profiling\n" +
                                  algos::EnumToAvailableValues<algos::PrimitiveType>() + " + [ac]";
    auto general_options = GeneralOptions();

    // clang-format off
    general_options.add_options()
            (kPrimitive, po::value<std::string>(&primitive)->required(), prim_desc.c_str());
    // clang-format on

    auto all_options = InfoOptions().add(general_options).add(AlgoOptions());
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, all_options), vm);
    } catch (po::error& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    if (vm.count(kHelp)) {
        std::cout << all_options << std::endl;
        return 0;
    }
    try {
        po::notify(vm);
    } catch (po::error& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    el::Loggers::configureFromGlobal("logging.conf");

    std::unique_ptr<algos::Primitive> algorithm_instance;
    try {
        algorithm_instance = algos::CreatePrimitive(primitive, vm);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    try {
        unsigned long long elapsed_time = algorithm_instance->Execute();
        std::cout << "> ELAPSED TIME: " << elapsed_time << std::endl;
    } catch (std::runtime_error& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}
