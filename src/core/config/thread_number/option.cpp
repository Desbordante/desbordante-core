#include "config/thread_number/option.h"

#include <string>
#include <thread>
#include <variant>

#include "common_option.h"
#include "config/exceptions.h"
#include "config/names_and_descriptions.h"
#include "thread_number/type.h"

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
