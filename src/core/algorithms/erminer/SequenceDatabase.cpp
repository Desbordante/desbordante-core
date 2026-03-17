#include "SequenceDatabase.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

void SequenceDatabase::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '%' || line[0] == '@') {
            continue;
        }
        
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string token;
        
        while (iss >> token) {
            tokens.push_back(token);
        }
        
        if (!tokens.empty()) {
            addSequence(tokens);
        }
    }
}

void SequenceDatabase::addSequence(const std::vector<std::string>& tokens) {
    auto sequence = std::make_unique<Sequence>(static_cast<int>(sequences.size()));
    std::vector<int> itemset;
    
    for (const auto& token : tokens) {
        if (!token.empty() && token[0] == '<') {
            continue;
        }
        else if (token == "-1") {
            if (!itemset.empty()) {
                sequence->addItemset(itemset);
                itemset.clear();
            }
        }
        else if (token == "-2") {
            if (!itemset.empty()){
                sequence->addItemset(itemset);
            }
            sequences.push_back(std::move(sequence));
            return;
        }
        else {
            try {
                itemset.push_back(std::stoi(token));
            } catch (const std::invalid_argument& e) {
                throw std::runtime_error("Invalid item in sequence: " + token);
            }
        }
    }
    
    if (!itemset.empty()) {
        sequence->addItemset(itemset);
    }
    sequences.push_back(std::move(sequence));
}

void SequenceDatabase::addSequence(std::unique_ptr<Sequence> sequence) {
    sequences.push_back(std::move(sequence));
}

int SequenceDatabase::size() const {
    return static_cast<int>(sequences.size());
}

const std::vector<std::unique_ptr<Sequence>>& SequenceDatabase::getSequences() const {
    return sequences;
}

std::unordered_set<int> SequenceDatabase::getSequenceIDs() const {
    std::unordered_set<int> ids;
    for (const auto& seq : sequences) {
        ids.insert(seq->getId());
    }
    return ids;
}

std::string SequenceDatabase::toString() const {
    std::stringstream ss;
    for (const auto& sequence : sequences) {
        ss << sequence->getId() << ":  " << sequence->toString() << "\n";
    }
    return ss.str();
}

void SequenceDatabase::printDatabaseStats() const {
    std::cout << "============  STATS ==========\n";
    std::cout << "Number of sequences : " << sequences.size() << "\n";
    
    if (sequences.empty()) {
        std::cout << "mean size: 0\n";
        return;
    }
    
    long totalSize = 0;
    for (const auto& seq : sequences) {
        totalSize += seq->size();
    }
    
    double meanSize = static_cast<double>(totalSize) / sequences.size();
    std::cout << "mean size: " << meanSize << "\n";
}