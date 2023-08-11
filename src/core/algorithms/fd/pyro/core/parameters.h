#pragma once

#include <string>

#include "config/equal_nulls/type.h"
#include "config/error/type.h"
#include "config/max_lhs/type.h"
#include "config/thread_number/type.h"

namespace pyro {

struct Parameters {
    bool is_find_keys = true;
    bool is_find_fds = true;
    config::EqNullsType is_null_equal_null = true;

    std::string ucc_error_measure = "g1prime";

    // Error settings
    config::ErrorType error_dev = 0;
    bool is_estimate_only = false;
    config::ErrorType max_ucc_error = 0.01;  // both for FD and UCC actually

    // Traversal settings
    config::ThreadNumType parallelism = 0;
    config::ThreadNumType max_threads_per_search_space = -1;
    bool is_defer_failed_launch_pads = true;
    std::string launch_pad_order = "error";

    config::MaxLhsType max_lhs = -1;

    // Sampling settings
    unsigned int sample_size = 10000;
    double sample_booster = 10;
    int seed = 0;                    // check seed, mb use boost::optional
    double estimate_confidence = 0;  //       -||-
    int random_ascend_threads = 2;

    // Cache settings
    double caching_probability = 0.5;
    unsigned int nary_intersection_size = 4;

    // Miscellaneous settings
    bool is_check_estimates = false;
    bool is_initial_pause = false;
    std::string fd_score_measure = "none";
};

}  // namespace pyro
