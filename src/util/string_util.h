#pragma once

#include <sstream>
#include <cstdarg>

[[maybe_unused]] std::string& ltrim(std::string &s);
[[maybe_unused]] std::string& rtrim(std::string &s);
[[maybe_unused]] std::string& trim(std::string &s);
[[maybe_unused]] std::string concat(int count, ...);
[[maybe_unused]] std::string concatCsv(int count, ...);
