#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include <memory>
#include "Sequence.hpp"


class SequenceDatabase {
private:
    std::vector<std::unique_ptr<Sequence>> sequences;
    
public:
    SequenceDatabase() = default;
    
    void loadFile(const std::string& path);
    
    void addSequence(std::unique_ptr<Sequence> sequence);
    
    int size() const;
    
    const std::vector<std::unique_ptr<Sequence>>& getSequences() const;
    
    std::unordered_set<int> getSequenceIDs() const;
    
    std::string toString() const;
    
    void printDatabaseStats() const;
    
private:
    void addSequence(const std::vector<std::string>& tokens);
};