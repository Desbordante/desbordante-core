#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "logging/easylogging++.h"

#include "algorithms/Pyro.h"
#include "algorithms/TaneX.h"
#include "algorithms/DFD/DFD.h"
#include "algorithms/FDep/FDep.h"
#include "algorithms/Fd_mine.h"
#include "algorithms/FastFDs.h"
#include "algorithms/depminer/Depminer.h"

namespace po = boost::program_options;

INITIALIZE_EASYLOGGINGPP

bool CheckOptions(std::string const& alg, double error) {
    if (alg != "pyro" && alg != "tane" && alg != "fastfds" && alg != "fdmine" && alg != "dfd" && alg != "depminer"
        && alg != "fdep") {
        std::cout << "ERROR: no matching algorithm. Available algorithms are:\n\tpyro\n\ttane.\n" << std::endl;
        return false;
    }
    if (error > 1 || error < 0) {
        std::cout << "ERROR: error should be between 0 and 1.\n" << std::endl;
    }
    return true;
}

int main(int argc, char const *argv[]) {
    std::string alg;
    std::string dataset;
    char separator = ',';
    bool has_header = true;
    int seed = 0;
    double error = 0.0;
    unsigned int max_lhs = -1;
    unsigned int parallelism = 0;

    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "print help")
            ("algo", po::value<std::string>(&alg), "algorithm [pyro|tane|fastfds|fdmine|dfd|fdep]")
            ("data", po::value<std::string>(&dataset), "path to CSV file, relative to ./inputData")
            ("sep", po::value<char>(&separator), "CSV separator")
            ("has_header", po::value<bool>(&has_header), "CSV header presence flag [true|false]. Default true")
            ("seed", po::value<int>(&seed), "RNG seed")
            ("error", po::value<double>(&error), "error for AFD algorithms. Default 0.01")
            ("maxLHS", po::value<unsigned int>(&max_lhs),
             (std::string("max considered LHS size. Default: ") + std::to_string((unsigned int)-1)).c_str())
            ("threads", po::value<unsigned int>(&parallelism))
            ;

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (po::error &e) {
        std::cout << e.what() << std::endl;
        return 0;
    }

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 0;
    }

    el::Loggers::configureFromGlobal("logging.conf");

    std::transform(alg.begin(), alg.end(), alg.begin(), [](unsigned char c){ return std::tolower(c); });

    if (!CheckOptions(alg, error)) {
        std::cout << desc << std::endl;
        return 1;
    }
    std::cout << "Input: algorithm \"" << alg
              << "\" with seed " << std::to_string(seed)
              << ", error \"" << std::to_string(error)
              << ", maxLHS \"" << std::to_string(max_lhs)
              << "\" and dataset \"" << dataset
              << "\" with separator \'" << separator
              << "\'. Header is " << (has_header ? "" : "not ") << "present. " << std::endl;
    auto path = std::filesystem::current_path() / "inputData" / dataset;

    std::unique_ptr<FDAlgorithm> algorithm_instance;
    if (alg == "pyro") {
        algorithm_instance = std::make_unique<Pyro>(path, separator, has_header, seed, error, max_lhs, parallelism);
    } else if (alg == "tane"){
        algorithm_instance = std::make_unique<Tane>(path, separator, has_header, error, max_lhs);
    } else if (alg == "dfd") {
        algorithm_instance = std::make_unique<DFD>(path, separator, has_header, parallelism);
    } else if (alg == "fdmine"){
        algorithm_instance = std::make_unique<Fd_mine>(path, separator, has_header);
    } else if (alg == "fastfds") {
        algorithm_instance = std::make_unique<FastFDs>(path, separator, has_header, max_lhs, parallelism);
    } else if (alg == "depminer") {
        algorithm_instance = std::make_unique<Depminer>(path, separator, has_header);
    } else if (alg == "fdep") {
        algorithm_instance = std::make_unique<FDep>(path, separator, has_header);
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
