#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include "../model/equivalence_class.h"
#include "../model/sequence.h"
#include "../model/types.h"

namespace algos::cmspade::parser {
class CMSpadeParser {
private:
    std::filesystem::path filepath_;
    std::ifstream file_;

public:
    CMSpadeParser(std::filesystem::path filepath);
    ~CMSpadeParser();

    CMSpadeParser(CMSpadeParser const&) = delete;
    CMSpadeParser& operator=(CMSpadeParser const&) = delete;

    CMSpadeParser(CMSpadeParser&&) = delete;
    CMSpadeParser& operator=(CMSpadeParser&&) = delete;

    std::vector<std::unique_ptr<Sequence>> ParseAll();

    bool ParseNextSequence(SequenceId sequence_id, std::unique_ptr<Sequence>& sequence);

private:
    void ParseSequenceLine(std::string const& line, SequenceId sequence_id,
                           std::unique_ptr<Sequence>& sequence);
};
}  // namespace algos::cmspade::parser