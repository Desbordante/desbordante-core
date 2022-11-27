#include "algorithms/options/info.h"

namespace algos::config {

OptionInfo::OptionInfo(std::string_view name, std::string_view description)
        : name_(name), description_(description) {}

std::string_view OptionInfo::GetName() const {
    return name_;
}

std::string_view OptionInfo::GetDescription() const {
    return description_;
}

}  // namespace algos::config
