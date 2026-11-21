#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/model/table/idataset_stream.h"

struct CSVConfig {
    std::filesystem::path path;
    char separator;
    bool has_header;

    static CSVConfig GetDefaultFor(std::filesystem::path const& path) {
        CSVConfig cfg = {
                .path = path,
                .separator = ',',
                .has_header = true,
        };

        return cfg;
    }
};

class CSVParser : public model::IDatasetStream {
private:
    class Reader;

    std::filesystem::path path_;
    char separator_;
    bool has_header_;
    std::unique_ptr<Reader> reader_;
    mutable std::optional<std::vector<std::string>> next_row_;
    std::vector<std::string> column_names_;
    std::string relation_name_;

    void InitializeReader();

public:
    CSVParser() = delete;
    explicit CSVParser(std::filesystem::path const& path);
    CSVParser(std::filesystem::path const& path, char separator, bool has_header);
    explicit CSVParser(CSVConfig const& csv_config);
    CSVParser(CSVParser&&) noexcept;
    CSVParser& operator=(CSVParser&&) noexcept;
    ~CSVParser() override;

    std::vector<std::string> GetNextRow() override;
    bool HasNextRow() const override;
    char GetSeparator() const;
    size_t GetNumberOfColumns() const override;
    std::string GetColumnName(size_t index) const override;
    std::string GetRelationName() const override;
    void Reset() override;
};
