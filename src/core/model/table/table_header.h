#pragma once

#include <string>
#include <vector>

namespace model {
struct TableHeader {
    std::string table_name;
    std::vector<std::string> column_names;
};
}  // namespace model
