#include <iostream>
#include <stdexcept>

#include <boost/program_options.hpp>
#include <easylogging++.h>

#include "algorithms/algo_factory.h"
#include "config/all_options.h"
#include "util/enum_to_available_values.h"

#include "algorithms/od/fastod/fastod.h"
#include "algorithms/od/fastod/data/data_frame.h"
#include <climits>
#include <string>
#include <filesystem>
#include <vector>
#include <fstream>

INITIALIZE_EASYLOGGINGPP

namespace {
constexpr auto kHelp = "help";
constexpr auto kAlgorithm = "algorithm";
constexpr auto kDHelp = "print the help message and exit";

// boost::program_options::options_description InfoOptions() {
//     namespace po = boost::program_options;
//     po::options_description info_options("Desbordante information options");
//     info_options.add_options()(kHelp, kDHelp)
//             // --version, if needed, goes here too
//             ;
//     return info_options;
// }
}  // namespace

int main(int argc, char const* argv[]) {
    std::string path_to_dataset = std::string("");
    std::string output_path = std::string("");  
    int max_time = INT_MAX;

    if (argc <= 1) {
        std::cout << "Error: Dataset not specified\n";
        return 1; 
    }
    if (argc > 4) {
        std::cout << "Error: Too many arguments entered\n";
        return 1;
    }

    if (argc >= 2)
        path_to_dataset = std::string(argv[1]);
    
    if (argc >= 3)
        output_path = std::string(argv[2]);

    if (argc >= 4) {
        max_time = std::stoi(argv[3]);
    }

    algos::fastod::DataFrame data = algos::fastod::DataFrame::FromCsv(std::filesystem::path(path_to_dataset));
    algos::fastod::Fastod fastod = algos::fastod::Fastod(data, max_time);
    std::vector<algos::fastod::CanonicalOD> ods = fastod.Discover();

    if (output_path.size() == 0) {
        std::cout << "Found ODs: " << ods.size() << '\n';

        for (size_t i = 0; i < ods.size(); ++i) {
            std::string od = ods[i].ToString();
            std::cout << i + 1 << ":\t" << od << '\n';
        }
    }
    else {
        std::ofstream file;
        file.open(output_path);

        if (!file.is_open()) {
            std::cout << "Error: failed to open output file\n";
            return 2;
        }

        for (size_t i = 0; i < ods.size(); ++i) {
            std::string od = ods[i].ToString();
            file << od << '\n';
        }

        file.close();
    }
}

/*
int main(int argc, char const* argv[]) {
    namespace po = boost::program_options;
    using namespace config;

    std::string algorithm;
    std::string const algo_desc = "algorithm to use for data profiling\n" +
                                  util::EnumToAvailableValues<algos::AlgorithmType>();
    auto general_options = GeneralOptions();

    // clang-format off
    general_options.add_options()
            (kAlgorithm, po::value<std::string>(&algorithm)->required(), algo_desc.c_str());
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

    std::unique_ptr<algos::Algorithm> algorithm_instance;
    try {
        algorithm_instance = algos::CreateAlgorithm(algorithm, vm);
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
*/
