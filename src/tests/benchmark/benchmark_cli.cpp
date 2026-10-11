#include "tests/benchmark/benchmark_cli.h"
#include <string>

#include <boost/program_options.hpp>
#include <boost/program_options/value_semantic.hpp>

namespace po = boost::program_options;

namespace benchmark {
BenchmarkCLI::BenchmarkCLI() {
    // clang-format off
	description_.add_options()
		(kHelpOption, "print help message")
		(kBaselineOption, po::value<std::string>(), "JSON with baseline benchmark results")
		(kOutputOption, po::value<std::string>(), "filename to save benchmark results")
		(kNameOption, po::value<std::string>(), "name of the benchmark(regex)")
		(kListOption, "list benchmark names");
    // clang-format on
}
}  // namespace benchmark
