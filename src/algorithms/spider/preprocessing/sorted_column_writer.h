#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <easylogging++.h>

namespace algos::ind::preproc {

/* A class that writes sorted columns to disk,
 * supporting partial column writes with final file merging.
 */
class SortedColumnWriter {
    std::filesystem::path temp_dir_;
    /* This field contains number of already processed columns */
    std::size_t processed_cols_number_ = 0;

    /* vector contains maximum value for attributes */
    std::vector<std::string> max_values_{};
    /* A field that keeps track of the number of times each attribute
     * in the current processed dataset has been written to disk.
     */
    std::vector<std::size_t> attributes_spill_counters_{};

    std::size_t GetCurrentColsNumber() const {
        return attributes_spill_counters_.size();
    }

    void Init(std::size_t cols_number);
    void Reset() {
        processed_cols_number_ += GetCurrentColsNumber();
        attributes_spill_counters_.clear();
    }

public:
    explicit SortedColumnWriter(std::filesystem::path const& temp_dir) : temp_dir_(temp_dir) {
        if (std::filesystem::exists(temp_dir)) {
            std::filesystem::remove_all(temp_dir);
        }
    }

    std::vector<std::string> GetMaxValues() const {
        return max_values_;
    }

    std::filesystem::path GetResultDirectory(std::optional<std::size_t> spilled_dir = std::nullopt);
    std::filesystem::path GetResultColumnPath(
            std::size_t id, std::optional<std::size_t> spilled_file_id = std::nullopt);

    static std::string MergeSpilledFiles(std::vector<std::ifstream> input, std::ofstream out);

    void MergeFiles();

    template <typename PrintFn, typename It>
    void WriteColumn(PrintFn const& print, bool spill_col, std::size_t attr_id, It begin, It end) {
        auto spilled_file = spill_col ? std::make_optional(attributes_spill_counters_[attr_id]++)
                                      : std::nullopt;
        auto path{GetResultColumnPath(processed_cols_number_ + attr_id, spilled_file)};
        std::ofstream out{path};

        if (!out.is_open()) {
            throw std::runtime_error("Cannot open file" + std::string{path});
        }
        while (begin != end) {
            print(out, *(begin++));
            out << std::endl;
        }
    }
    template <typename PrintFn, typename T>
    void SpillColumnsToDisk(PrintFn const& print, T& columns, bool is_table_splitted = false) {
        Init(columns.size());
        auto init_time = std::chrono::system_clock::now();

        for (std::size_t attr_id = 0; attr_id != columns.size(); ++attr_id) {
            bool is_column_part = is_table_splitted || attributes_spill_counters_[attr_id] > 0;
            auto& values = columns[attr_id];
            WriteColumn(print, is_column_part, attr_id, values.begin(), values.end());
            values.clear();
        }
        auto writing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - init_time);
        LOG(INFO) << "writing time: " << writing_time.count();
    }
};

}  // namespace algos::ind::preproc
