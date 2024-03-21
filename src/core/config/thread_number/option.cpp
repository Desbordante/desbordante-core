#include "config/thread_number/option.h"

#include <thread>

#include "config/exceptions.h"
#include "config/names_and_descriptions.h"

namespace config {
using names::kThreads, descriptions::kDThreads;
extern CommonOption<ThreadNumType> const kThreadNumberOpt{
        kThreads, kDThreads, 0, [](auto &value) {
            if (value == 0) {
                value = std::thread::hardware_concurrency();
                if (value == 0) {
                    throw ConfigurationError(
                            "Unable to detect number of concurrent threads supported by your "
                            "system. Please, specify it manually.");
                }
            }
        }};
}  // namespace config
