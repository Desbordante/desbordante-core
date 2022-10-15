#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <easylogging++.h>

#include "algo_factory.h"
#include "program_option_strings.h"

namespace po = boost::program_options;
namespace posr = program_option_strings;

INITIALIZE_EASYLOGGINGPP

template<typename BetterEnumType>
static std::string EnumToAvailableValues() {
    std::stringstream avail_values;

    avail_values << '[';

    for (auto const& name : BetterEnumType::_names()) {
        avail_values << name << '|';
    }

    avail_values.seekp(-1, avail_values.cur);
    avail_values << ']';

    return avail_values.str();
}

static bool CheckOptions(std::string const& task, std::string const& alg, std::string const& metric,
                         std::string const& metric_alg, size_t rhs_indices_count, double error) {
    if (!algos::AlgoMiningType::_is_valid(task.c_str())) {
        std::cout << "ERROR: no matching task."
                     " Available tasks (primitives to mine) are:\n" +
                     EnumToAvailableValues<algos::AlgoMiningType>() + '\n';
        return false;
    }

    if (task == "metric") {
        if (!algos::Metric::_is_valid(metric.c_str())) {
            std::cout << "ERROR: no matching metric."
                         " Available metrics are:\n" +
                         EnumToAvailableValues<algos::Metric>() + '\n';
            return false;
        }
        if (rhs_indices_count == 1 && metric == "euclidean") {
            if (!metric_alg.empty()) {
                std::cout << "metric_algo is ignored for one-dimensional euclidean metric.\n";
            }
            return true;
        }
        if (!algos::MetricAlgo::_is_valid(metric_alg.c_str())) {
            std::cout << "ERROR: no matching MFD algorithm."
                         " Available MFD algorithms are:\n" +
                         EnumToAvailableValues<algos::MetricAlgo>() + '\n';
            return false;
        }
        return true;
    }

    if (!algos::Algo::_is_valid(alg.c_str())) {
        std::cout << "ERROR: no matching algorithm."
                     " Available algorithms are:\n" +
                     EnumToAvailableValues<algos::Algo>() + '\n';
        return false;
    }

    if (error > 1 || error < 0) {
        std::cout << "ERROR: error should be between 0 and 1.\n";
    }

    return true;
}

int main(int argc, char const* argv[]) {
    std::string algo;
    std::string dataset;
    std::string task;
    char separator = ',';
    bool has_header = true;
    int seed = 0;
    double error = 0.0;
    unsigned int max_lhs = -1;
    ushort threads = 0;
    bool is_null_equal_null = true;

    /*Options for association rule mining algorithms*/
    double minsup = 0.0;
    double minconf = 0.0;
    std::string ar_input_format;
    unsigned tid_column_index = 0;
    unsigned item_column_index = 1;
    bool has_transaction_id = false;

    /*Options for metric verifier algorithm*/
    std::string metric;
    std::string metric_algo;
    std::vector<unsigned int> lhs_indices;
    std::vector<unsigned int> rhs_indices;
    long double parameter = 0;
    unsigned int q = 2;
    bool dist_to_null_infinity = false;

    std::string const algo_desc = "algorithm to use. Available algorithms:\n" +
                                  EnumToAvailableValues<algos::Algo>() +
                                  " for FD mining.";
    std::string const task_desc = "type of dependency to mine. Available tasks:\n" +
                                  EnumToAvailableValues<algos::AlgoMiningType>();
    std::string const metric_desc = "metric to use. Available metrics:\n" +
        EnumToAvailableValues<algos::Metric>();
    std::string const metric_algo_desc = "MFD algorithm to use. Available algorithms:\n" +
        EnumToAvailableValues<algos::MetricAlgo>();

    po::options_description info_options("Desbordante information options");
    info_options.add_options()
        (posr::kHelp, "print the help message and exit")
        // --version, if needed, goes here too
        ;

    po::options_description general_options("General options");
    general_options.add_options()
        (posr::kTask, po::value<std::string>(&task), task_desc.c_str())
        (posr::kAlgorithm, po::value<std::string>(&algo), algo_desc.c_str())
        (posr::kData, po::value<std::string>(&dataset),
            "path to CSV file, relative to ./input_data")
        (posr::kSeparatorLibArg, po::value<char>(&separator)->default_value(separator),
            "CSV separator")
        (posr::kHasHeader, po::value<bool>(&has_header)->default_value(has_header),
         "CSV header presence flag [true|false]")
        (posr::kEqualNulls, po::value<bool>(&is_null_equal_null)->default_value(true),
         "specify whether two NULLs should be considered equal")
        (posr::kThreads, po::value<ushort>(&threads)->default_value(threads),
         "number of threads to use. If 0, then as many threads are used as "
         "the hardware can handle concurrently.")
        ;

    po::options_description typos_fd_options("Typo mining/FD options");
    typos_fd_options.add_options()
        (posr::kError, po::value<double>(&error)->default_value(error),
         "error value for AFD algorithms")
        (posr::kMaximumLhs, po::value<unsigned int>(&max_lhs)->default_value(max_lhs),
         "max considered LHS size")
        (posr::kSeed, po::value<int>(&seed)->default_value(seed), "RNG seed")
        ;

    po::options_description ar_options("AR options");
    ar_options.add_options()
        (posr::kMinimumSupport, po::value<double>(&minsup),
            "minimum support value (between 0 and 1)")
        (posr::kMinimumConfidence, po::value<double>(&minconf),
            "minimum confidence value (between 0 and 1)")
        (posr::kInputFormat, po::value<string>(&ar_input_format),
         "format of the input dataset. [singular|tabular] for AR mining")
        ;

    po::options_description ar_singular_options("AR \"singular\" input format options");
    ar_singular_options.add_options()
        (posr::kTIdColumnIndex, po::value<unsigned>(&tid_column_index)->default_value(0),
         "index of the column where a TID is stored")
        (posr::kItemColumnIndex, po::value<unsigned>(&item_column_index)->default_value(1),
         "index of the column where an item name is stored")
        ;

    po::options_description ar_tabular_options("AR \"tabular\" input format options");
    ar_tabular_options.add_options()
        (posr::kFirstColumnTId, po::bool_switch(&has_transaction_id),
         "indicates that the first column contains the transaction IDs")
        ;

    ar_options.add(ar_singular_options).add(ar_tabular_options);

    po::options_description mfd_options("MFD options");
    mfd_options.add_options()
        (posr::kMetric, po::value<std::string>(&metric), metric_desc.c_str())
        (posr::kMetricAlgorithm, po::value<std::string>(&metric_algo), metric_algo_desc.c_str())
        (posr::kLhsIndices, po::value<std::vector<unsigned int>>(&lhs_indices)->multitoken(),
         "LHS column indices for metric FD verification")
        (posr::kRhsIndices, po::value<std::vector<unsigned int>>(&rhs_indices)->multitoken(),
         "RHS column indices for metric FD verification")
        (posr::kParameter, po::value<long double>(&parameter), "metric FD parameter")
        (posr::kDistToNullIsInfinity, po::bool_switch(&dist_to_null_infinity),
         "specify whether distance to NULL value is infinity (otherwise it is 0)")
        ;

    po::options_description cosine_options("Cosine metric options");
    cosine_options.add_options()
        (posr::kQGramLength, po::value<unsigned int>(&q)->default_value(2),
         "q-gram length for cosine metric")
        ;

    mfd_options.add(cosine_options);

    po::options_description all_options("Allowed options");
    all_options.add(info_options).add(general_options).add(typos_fd_options)
        .add(mfd_options).add(ar_options);

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, all_options), vm);
        po::notify(vm);
    } catch (po::error &e) {
        std::cout << e.what() << std::endl;
        return 0;
    }

    if (vm.count(posr::kHelp))
    {
        std::cout << all_options << std::endl;
        return 0;
    }

    el::Loggers::configureFromGlobal("logging.conf");

    std::transform(algo.begin(), algo.end(), algo.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (task == "metric") {
        auto remove_duplicates = [](std::vector<unsigned>& vec) {
            if (vec.empty()) {
                throw std::invalid_argument("Index vector should not be empty.");
            }
            std::sort(vec.begin(), vec.end());
            vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
        };
        remove_duplicates(lhs_indices);
        remove_duplicates(rhs_indices);
        vm.at("rhs_indices").value() = rhs_indices;
        vm.at("lhs_indices").value() = lhs_indices;
    }

    if (!CheckOptions(task, algo, metric, metric_algo, rhs_indices.size(), error)) {
        std::cout << all_options << std::endl;
        return 1;
    }

    if (task == "fd" || task == "typos") {
        std::cout << "Input: algorithm \"" << algo
                  << "\" with seed " << std::to_string(seed)
                  << ", error \"" << std::to_string(error)
                  << ", max_lhs \"" << std::to_string(max_lhs)
                  << "\" and dataset \"" << dataset
                  << "\" with separator \'" << separator
                  << "\'. Header is " << (has_header ? "" : "not ") << "present. " << std::endl;
    } else if (task == "ar") {
        std::cout << "Input: algorithm \"" << algo
                  << "\" with min. support threshold \"" << std::to_string(minsup)
                  << "\", min. confidence threshold \"" << std::to_string(minconf)
                  << "\" and dataset \"" << dataset
                  << "\". Input type is \"" << ar_input_format
                  << "\" with separator \'" << separator
                  << "\'. Header is " << (has_header ? "" : "not ") << "present. " << std::endl;
    } else if (task == "metric") {
        algo = "metric";
        auto get_str = [](std::vector<unsigned> const& vec) {
            std::stringstream stream;
            std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(stream, " "));
            string lhs_indices_str = stream.str();
            boost::trim_right(lhs_indices_str);
            return lhs_indices_str;
        };
        std::cout << "Input metric \"" << metric;
        if (metric == "cosine") std::cout << "\" with q \"" << q;
        std::cout << "\" with parameter \"" << parameter
                  << "\" with LHS indices \"" << get_str(lhs_indices)
                  << "\" with RHS indices \"" << get_str(rhs_indices)
                  << "\" and dataset \"" << dataset
                  << "\" with separator \'" << separator
                  << "\'. Header is " << (has_header ? "" : "not ") << "present. " << std::endl;
    }

    std::unique_ptr<algos::Primitive> algorithm_instance =
        algos::CreateAlgorithmInstance(task, algo, vm);

    try {
        unsigned long long elapsed_time = algorithm_instance->Execute();
        std::cout << "> ELAPSED TIME: " << elapsed_time << std::endl;
    } catch (std::runtime_error& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}
