#include "file_sequence_parser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace parser {

namespace {
std::vector<std::string> SplitString(std::string const& str) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream token_stream(str);
    while (token_stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

model::Event ParseEvent(std::string const& str) {
    try {
        return std::stoull(str);
    } catch (std::exception const& e) {
        throw std::runtime_error("Invalid event format in file: expected unsigned int, got \"" +
                                 str + "\"");
    }
}

model::Timestamp ParseTimestamp(std::string const& str) {
    try {
        return std::stoull(str);
    } catch (std::exception const& e) {
        throw std::runtime_error("Invalid timestamp format in file: expected unsigned int, got \"" +
                                 str + "\"");
    }
}
}  // namespace

FileSequenceParser::FileSequenceParser(std::filesystem::path path) : path_(std::move(path)) {
    file_.open(path_);
    if (!file_.is_open()) {
        throw std::runtime_error("Could not open file: " + path_.string());
    }
    current_line_number_ = 0;
    expect_explicit_timestamp_ = std::nullopt;
    has_buffered_line_ = false;
    current_line_.clear();
}

bool FileSequenceParser::PeekNextLine() {
    if (has_buffered_line_) return true;

    while (std::getline(file_, current_line_)) {
        // Check if line contains non-whitespace characters
        auto it = std::find_if(current_line_.begin(), current_line_.end(),
                               [](unsigned char ch) { return !std::isspace(ch); });

        if (it != current_line_.end()) {
            has_buffered_line_ = true;
            return true;
        }
    }
    return false;
}

bool FileSequenceParser::HasNext() {
    return PeekNextLine();
}

model::TimedEventSet FileSequenceParser::GetNext() {
    if (!has_buffered_line_) {
        if (!PeekNextLine()) {
            throw std::runtime_error("No more records.");
        }
    }

    size_t pipe_pos = current_line_.find('|');
    bool has_explicit_timestamp = (pipe_pos != std::string::npos);

    if (!expect_explicit_timestamp_.has_value()) {
        expect_explicit_timestamp_ = has_explicit_timestamp;
    } else if (*expect_explicit_timestamp_ != has_explicit_timestamp) {
        throw std::runtime_error(
                "Inconsistent sequence data in file: mixed explicit and implicit timestamps.");
    }

    std::vector<model::Event> events;
    model::Timestamp timestamp;

    if (has_explicit_timestamp) {
        std::string events_part = current_line_.substr(0, pipe_pos);
        std::string timestamp_part = current_line_.substr(pipe_pos + 1);

        auto event_tokens = SplitString(events_part);
        for (auto const& token : event_tokens) {
            events.push_back(ParseEvent(token));
        }

        timestamp = ParseTimestamp(timestamp_part);
    } else {
        timestamp = current_line_number_;
        auto event_tokens = SplitString(current_line_);
        for (auto const& token : event_tokens) {
            events.push_back(ParseEvent(token));
        }
    }

    current_line_number_++;
    has_buffered_line_ = false;
    current_line_.clear();

    model::TimedEventSet record{std::move(events), timestamp};
    if (!record.IsSortedUnique()) {
        throw std::runtime_error("Events in a transaction are not sorted or contain duplicates.");
    }
    return record;
}

}  // namespace parser
