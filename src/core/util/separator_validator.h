#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "parser/csv_parser/csv_parser.h"

namespace util {

std::pair<std::optional<char>, std::string> ValidateSeparator(std::filesystem::path const& path,
                                                              char separator);

}  // namespace util
