#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include "core/model/sequence/isequence_stream.h"

namespace parser {

class FileSequenceParser : public model::ISequenceStream {
public:
    explicit FileSequenceParser(std::filesystem::path path);
    bool HasNext() override;
    model::TimedEventSet GetNext() override;

private:
    std::filesystem::path path_;
    std::ifstream file_;
    std::string current_line_;
    bool has_buffered_line_ = false;
    size_t current_line_number_ = 0;
    std::optional<bool> expect_explicit_timestamp_;

    bool PeekNextLine();
};

}  // namespace parser
