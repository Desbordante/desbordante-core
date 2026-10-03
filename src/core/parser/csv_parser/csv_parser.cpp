#include "core/parser/csv_parser/csv_parser.h"

#include <internal/csv_reader.hpp>
#include <internal/csv_row.hpp>

class CSVParser::Reader {
public:
    Reader(std::filesystem::path const& path, char separator, bool has_header) {
        Reset(path, separator, has_header);
    }

    void Reset(std::filesystem::path const& path, char separator, bool has_header) {
        value.emplace(path.string(),
                      csv::CSVFormat().delimiter(separator).header_row(has_header ? 0 : -1));
    }

    std::optional<csv::CSVReader> value;
};

CSVParser::CSVParser(std::filesystem::path const& path)
    : CSVParser(CSVConfig::GetDefaultFor(path)) {}

CSVParser::CSVParser(std::filesystem::path const& path, char separator, bool has_header)
    : CSVParser(CSVConfig{path, separator, has_header}) {}

CSVParser::CSVParser(CSVConfig const& csv_config)
    : path_(csv_config.path),
      separator_(csv_config.separator),
      has_header_(csv_config.has_header),
      relation_name_(csv_config.path.filename()) {
    InitializeReader();
}

CSVParser::CSVParser(CSVParser&&) noexcept = default;

CSVParser& CSVParser::operator=(CSVParser&&) noexcept = default;

CSVParser::~CSVParser() = default;

void CSVParser::InitializeReader() {
    if (reader_) {
        reader_->Reset(path_, separator_, has_header_);
    } else {
        reader_ = std::make_unique<Reader>(path_, separator_, has_header_);
    }
    next_row_.reset();
    if (has_header_) {
        column_names_ = reader_->value->get_col_names();
        return;
    }

    if (!HasNextRow()) {
        column_names_.clear();
        return;
    }
    column_names_.resize(next_row_->size());
    for (std::size_t i = 0; i != column_names_.size(); ++i) {
        column_names_[i] = std::to_string(i);
    }
}

std::vector<std::string> CSVParser::GetNextRow() {
    if (!HasNextRow()) {
        return {};
    }
    auto row = std::move(*next_row_);
    next_row_.reset();
    return row;
}

bool CSVParser::HasNextRow() const {
    if (next_row_.has_value()) {
        return true;
    }
    csv::CSVRow row;
    if (!reader_->value->read_row(row)) {
        return false;
    }
    next_row_ = static_cast<std::vector<std::string>>(row);
    return true;
}

char CSVParser::GetSeparator() const {
    return separator_;
}

size_t CSVParser::GetNumberOfColumns() const {
    return column_names_.size();
}

std::string CSVParser::GetColumnName(size_t index) const {
    return column_names_.at(index);
}

std::string CSVParser::GetRelationName() const {
    return relation_name_;
}

void CSVParser::Reset() {
    InitializeReader();
}
