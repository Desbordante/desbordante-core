#pragma once

#include <boost/program_options.hpp>

namespace algos::config {
boost::program_options::options_description GeneralOptions();
boost::program_options::options_description AlgoOptions();
}  // namespace algos::config
