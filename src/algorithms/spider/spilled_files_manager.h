#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace algos {

class SpilledFilesManager {
    std::filesystem::path temp_dir_;
    // Current offset
    std::size_t cur_offset_ = 0;

    /* i-nth element contains maximum value for i-nth attribute */
    std::vector<std::string> max_values_{};
    /* i-nth element contains spill counter for current i-nth attribute
     * (zero if there is no spills for i-nth attribute) */
    std::vector<std::size_t> spill_count_{};

    std::size_t GetCurrentColsNumber() const {
        return spill_count_.size();
    }

    void Init(std::size_t cols_number);
    void Reset() {
        cur_offset_ += GetCurrentColsNumber();
        spill_count_.clear();
    }

public:
    explicit SpilledFilesManager(std::filesystem::path const& temp_dir) : temp_dir_(temp_dir) {
        if (std::filesystem::exists(temp_dir)) {
            std::filesystem::remove_all(temp_dir);
        }
    }

    std::vector<std::string> GetMaxValues() const {
        return max_values_;
    }

    static std::filesystem::path GetResultDirectory(
            std::optional<std::size_t> spilled_dir = std::nullopt);

    static std::filesystem::path GetResultColumnPath(
            std::size_t id, std::optional<std::size_t> spilled_file_id = std::nullopt);

    static std::string MergeSpilledFiles(std::vector<std::ifstream> input, std::ofstream out);

    void MergeFiles();

    template <typename It, typename Fn>
    void WriteColumn(Fn const& to_value, bool spill_col, std::size_t attr_id, It begin, It end) {
        auto spilled_file = spill_col ? std::make_optional(spill_count_[attr_id]++) : std::nullopt;
        auto path{GetResultColumnPath(cur_offset_ + attr_id, spilled_file)};
        std::ofstream out{path};

        if (!out.is_open()) {
            throw std::runtime_error("Cannot open file" + std::string{path});
        }
        while (begin != end) {
            out << to_value(*(begin++)) << std::endl;
        }
    }
    template <typename Fn, typename T>
    void SpillColumnsToDisk(Fn const& to_value, T& columns, bool is_table_splitted = false) {
        Init(columns.size());
        for (std::size_t attr_id = 0; attr_id != columns.size(); ++attr_id) {
            bool is_column_part = is_table_splitted || spill_count_[attr_id] > 0;
            auto& values = columns[attr_id];
            WriteColumn(to_value, is_column_part, attr_id, values.begin(), values.end());
            values.clear();
        }
    }
};

}  // namespace algos
