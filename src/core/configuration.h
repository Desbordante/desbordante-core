#pragma once

#include <string>

#include "algorithms/options/equal_nulls/type.h"
#include "algorithms/options/error/type.h"
#include "algorithms/options/max_lhs/type.h"
#include "algorithms/options/thread_number/type.h"

struct Configuration {
    bool is_find_keys = true;
    bool is_find_fds = true;
    algos::config::EqNullsType is_null_equal_null = true;

    std::string ucc_error_measure = "g1prime";

    //Error settings
    algos::config::ErrorType error_dev = 0;
    bool is_estimate_only = false;
    algos::config::ErrorType max_ucc_error = 0.01;  // both for FD and UCC actually

    //Traversal settings
    algos::config::ThreadNumType parallelism = 0;
    algos::config::ThreadNumType max_threads_per_search_space = -1;
    bool is_defer_failed_launch_pads = true;
    std::string launch_pad_order = "error";

    algos::config::MaxLhsType max_lhs = -1;

    //Sampling settings
    unsigned int sample_size = 10000;
    double sample_booster = 10;
    int seed = 0; //check seed, mb use boost::optional
    double estimate_confidence = 0; //       -||-
    int random_ascend_threads = 2;

    //Cache settings
    double caching_probability = 0.5;
    unsigned int nary_intersection_size = 4;

    //Miscellaneous settings
    bool is_check_estimates = false;
    bool is_initial_pause = false;
    std::string fd_score_measure = "none";
};