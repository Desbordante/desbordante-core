#pragma once

#include <thread>

#include "algorithms/options/descriptions.h"
#include "algorithms/options/names.h"
#include "algorithms/options/type.h"

namespace algos::config {

using ThreadNumType = ushort;
const OptionType<ThreadNumType> ThreadNumberOpt{
        {names::kThreads, descriptions::kDThreads}, 0, [](auto &value) {
            if (value == 0) {
                value = std::thread::hardware_concurrency();
                if (value == 0) {
                    throw std::runtime_error(
                            "Unable to detect number of concurrent "
                            "threads supported by your system. "
                            "Please, specify it manually.");
                }
            }
        }};

}  // namespace algos::config
