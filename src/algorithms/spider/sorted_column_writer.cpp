#include "sorted_column_writer.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <queue>

#include <easylogging++.h>

namespace algos {

void SortedColumnWriter::Init(std::size_t cols_number) {
    if (attributes_spill_counters_.empty()) {
        max_values_.resize(max_values_.size() + cols_number);
        attributes_spill_counters_.assign(cols_number, 0);
    }
}

std::filesystem::path SortedColumnWriter::GetResultDirectory(
        std::optional<std::size_t> spilled_dir) {
    std::filesystem::path path;
    if (spilled_dir.has_value()) {
        path = GetResultDirectory() / (std::to_string(spilled_dir.value()) + "_col");
    } else {
        path = std::filesystem::current_path() / temp_dir_;
    }
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directory(path);
    }
    return path;
}

std::filesystem::path SortedColumnWriter::GetResultColumnPath(
        std::size_t id, std::optional<std::size_t> spilled_file_id) {
    if (spilled_file_id.has_value()) {
        return GetResultDirectory(id) / std::to_string(spilled_file_id.value());
    } else {
        return GetResultDirectory() / std::to_string(id);
    }
}

std::string SortedColumnWriter::MergeSpilledFiles(std::vector<std::ifstream> input,
                                                  std::ofstream out) {
    using ColumnElement = std::pair<std::string, std::size_t>;
    auto cmp = [](ColumnElement const& lhs, ColumnElement const& rhs) {
        return lhs.first > rhs.first;
    };
    std::priority_queue<ColumnElement, std::vector<ColumnElement>, decltype(cmp)> queue{cmp};
    for (std::size_t i = 0; i < input.size(); ++i) {
        std::string value;
        if (std::getline(input[i], value)) {
            queue.emplace(value, i);
        }
    }
    std::string prev;
    while (!queue.empty()) {
        auto [value, file_index] = queue.top();
        queue.pop();
        if (value != prev) {
            out << (prev = value) << std::endl;
        }
        if (std::getline(input[file_index], value)) {
            queue.emplace(value, file_index);
        }
    }
    return prev;
}

void SortedColumnWriter::MergeFiles() {
    auto& counters = attributes_spill_counters_;
    if (!std::all_of(counters.begin(), counters.end(), [](auto i) { return i == 0; })) {
        auto merge_time = std::chrono::system_clock::now();

        for (std::size_t i = 0; i != GetCurrentColsNumber(); ++i) {
            if (counters[i] == 0) {
                continue;
            }
            std::vector<std::ifstream> input_files{};
            for (std::size_t j = 0; j != counters[i]; ++j) {
                input_files.emplace_back(GetResultColumnPath(processed_cols_number_ + i, j));
            }
            max_values_[processed_cols_number_ + i] = MergeSpilledFiles(
                    std::move(input_files), GetResultColumnPath(processed_cols_number_ + i));
            std::filesystem::remove_all(GetResultDirectory(processed_cols_number_ + i));
        }

        auto merging_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - merge_time);
        LOG(INFO) << "Merging time: " << merging_time.count();
    }
    Reset();
}

}  // namespace algos
