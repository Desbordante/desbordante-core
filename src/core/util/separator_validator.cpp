#include "separator_validator.h"

namespace util {

std::pair<std::optional<char>, std::string> ValidateSeparator(std::filesystem::path const& path,
                                                              char separator) {
    auto parser = std::make_unique<CSVParser>(path, separator, false);
    return parser->ValidateSeparator();
}

}  // namespace util
