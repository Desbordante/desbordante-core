#pragma once 

#include "../model/types.h"
#include "../model/sequence.h"
#include "../model/equivalence_class.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <memory>
#include <unordered_map>
namespace algos::cmspade::parser{
class CMSpadeParser{
private:
    std::filesystem::path filepath_;
    std::ifstream file_;
public: 
    CMSpadeParser(std::filesystem::path filepath);
    ~CMSpadeParser();

    CMSpadeParser(const CMSpadeParser&) = delete;
    CMSpadeParser& operator=(const CMSpadeParser&) = delete;

    CMSpadeParser(CMSpadeParser&&) = delete;
    CMSpadeParser& operator=(CMSpadeParser&&) = delete;

    std::vector<std::unique_ptr<Sequence>> ParseAll();

    bool ParseNextSequence(SequenceId sequence_id, 
                         std::unique_ptr<Sequence>& sequence);

private:
    void ParseSequenceLine(const std::string& line, SequenceId sequence_id, 
                            std::unique_ptr<Sequence>& sequence);

};
} // namespace algos::cmspade::parser