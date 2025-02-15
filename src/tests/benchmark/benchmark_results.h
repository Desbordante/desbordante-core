#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include <boost/json/src.hpp>

namespace benchmark::util {

class BenchmarkResults {
public:
    /// @brief Load benchmark results saved with @c Save().
    /// @param filename must point to a @b valid JSON of exact this format: {"HyFD": 30000, ...}
    /// @note Some checks are performed, but don't expect too much.
    static std::unordered_map<std::string, long long> Load(std::string_view filename) {
        std::filesystem::path file_path{filename};
        std::ifstream file{file_path};

        // This thing will throw file's stored exception, if one
        file.exceptions(file.rdstate());

        // This form of parse throws exceptions
        boost::json::value value = boost::json::parse(file);
        file.close();

        std::unordered_map<std::string, long long> results;
        if (!value.is_object()) {
            throw std::logic_error{"Load: Top-level value of JSON must be an object"};
        }
        auto obj = value.get_object();
        for (auto const& [key, value] : obj) {
            // Key is intended to be algo name, value -- its execution time
            long long time;
            if (value.is_int64()) {
                time = value.as_int64();
            } else if (value.is_uint64()) {
                time = value.as_uint64();
            } else {
                std::ostringstream msg;
                msg << "Load: Time of algorithm " << key << " must be integer, got "
                    << value.kind();
                throw std::logic_error(msg.str());
            }
            results.emplace(key, time);
        }
        return results;
    }

    /// @brief Save benchmark results to be read with Load() later.
    /// @note Some checks are performed, but don't expect too much.
    static void Save(std::unordered_map<std::string, long long> const& results,
                     std::string_view filename) {
        std::filesystem::path file_path{filename};
        std::ofstream file{file_path};

        // This thing will throw file's stored exception, if one
        file.exceptions(file.rdstate());

        boost::json::object top_level_obj{results.begin(), results.end()};
        file << top_level_obj << '\n';
    }
};

}  // namespace benchmark::util
