#include "algorithms/options/thread_number/option.h"

#include <thread>

#include "algorithms/options/names_and_descriptions.h"

namespace algos::config {
using names::kThreads, descriptions::kDThreads;
extern const CommonOption<ThreadNumType> ThreadNumberOpt{
        kThreads, kDThreads, 0, [](auto &value) {
            if (value == 0) {
                value = std::thread::hardware_concurrency();
                if (value == 0) {
                    throw std::runtime_error(
                            "Unable to detect number of concurrent threads "
                            "supported by your system. Please, specify it "
                            "manually.");
                }
            }
        }};
}  // namespace algos::config
