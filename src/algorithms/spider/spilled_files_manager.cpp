#include "spilled_files_manager.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <queue>

#include <easylogging++.h>

namespace algos {
void SpilledFilesManager::Init(std::size_t cols_number) {
    if (spill_count_.empty()) {
        max_values_.resize(max_values_.size() + cols_number);
        spill_count_.assign(cols_number, 0);
    }
}

std::filesystem::path SpilledFilesManager::GetResultDirectory(
        std::optional<std::size_t> spilled_dir) {
    std::filesystem::path path;
    if (spilled_dir.has_value()) {
        path = GetResultDirectory() / (std::to_string(spilled_dir.value()) + "-column");
    } else {
        path = std::filesystem::current_path() / "temp";
    }
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directory(path);
    }
    return path;
}

std::filesystem::path SpilledFilesManager::GetResultColumnPath(
        std::size_t id, std::optional<std::size_t> spilled_file_id) {
    if (spilled_file_id.has_value()) {
        return GetResultDirectory(id) / std::to_string(spilled_file_id.value());
    } else {
        return GetResultDirectory() / std::to_string(id);
    }
}

std::string SpilledFilesManager::MergeSpilledFiles(std::vector<std::ifstream> input,
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

void SpilledFilesManager::MergeFiles() {
    if (!std::all_of(spill_count_.begin(), spill_count_.end(), [](auto i) { return i == 0; })) {
        auto merge_time = std::chrono::system_clock::now();

        for (std::size_t i = 0; i != GetCurrentColsNumber(); ++i) {
            if (spill_count_[i] == 0) {
                continue;
            }
            std::vector<std::ifstream> input_files{};
            for (std::size_t j = 0; j != spill_count_[i]; ++j) {
                input_files.emplace_back(GetResultColumnPath(cur_offset_ + i, j));
            }
            max_values_[cur_offset_ + i] =
                    MergeSpilledFiles(std::move(input_files), GetResultColumnPath(cur_offset_ + i));
            std::filesystem::remove_all(GetResultDirectory(cur_offset_ + i));
        }

        auto merging_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - merge_time);
        LOG(INFO) << "Merging time: " << merging_time.count();
    }
    Reset();
}

}  // namespace algos
