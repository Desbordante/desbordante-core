#pragma once

#include <iostream>

#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace benchmark {
constexpr static auto kHelpOption = "help,h";
constexpr static auto kHelpLongOption = "help";
constexpr static auto kBaselineOption = "baseline,b";
constexpr static auto kBaselineLongOption = "baseline";
constexpr static auto kOutputOption = "output,o";
constexpr static auto kOutputLongOption = "output";

class BenchmarkCLI {
private:
    constexpr static auto kHelpMsg =
            "Benchmark runner and comparer\n"
            "Usage: Desbordante_benchmark [--baseline <baseline filename>] [--output <output "
            "filename>]\n"
            "Available options";

    boost::program_options::options_description description_{kHelpMsg};
    boost::program_options::variables_map var_map_;

public:
    BenchmarkCLI();

    void ParseOptions(int argc, char* argv[]) {
        boost::program_options::store(
                boost::program_options::parse_command_line(argc, argv, description_), var_map_);
    }

    boost::program_options::variables_map const& GetVariablesMap() const {
        return var_map_;
    }

    void PrintHelp() const {
        std::cout << description_;
    }
};
}  // namespace benchmark
