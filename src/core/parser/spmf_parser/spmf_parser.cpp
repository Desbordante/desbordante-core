#include "spmf_parser.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace parser {

SPMFParser::SPMFParser(std::filesystem::path const& path) : path_(path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File not found: " + path.string());
    }
}

SPMFSequenceData SPMFParser::Parse(const std::set<int>& must_appear_items) {
    SPMFSequenceData result;
    result.sequence_starts.clear();
    result.total_itemsets = 0;
    result.num_sequences = 0;
    
    std::ifstream file(path_);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path_.string());
    }

    // First pass: collect all itemsets and build database structure
    std::vector<std::vector<std::vector<int>>> db;
    std::string line;
    int global_itemset_count = 0;

    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == '%' || line[0] == '@') {
            continue;
        }

        std::istringstream iss(line);
        std::string token;
        std::vector<std::vector<int>> sequence;
        std::vector<int> current_itemset;
        bool has_must_item = must_appear_items.empty();

        // Parse sequence
        while (iss >> token) {
            int val = std::stoi(token);
            if (val == -1) {
                // End of itemset
                if (!current_itemset.empty()) {
                    sequence.emplace_back(std::move(current_itemset));
                    current_itemset.clear();
                    global_itemset_count++;
                }
            } else if (val == -2) {
                // End of sequence
                break;
            } else {
                // Regular item
                current_itemset.push_back(val);
                if (!has_must_item && must_appear_items.count(val)) {
                    has_must_item = true;
                }
            }
        }
        
        // Handle last itemset if present
        if (!current_itemset.empty()) {
            sequence.emplace_back(std::move(current_itemset));
            global_itemset_count++;
        }

        // Add sequence only if it has required items (or no restriction)
        if (has_must_item && !sequence.empty()) {
            db.push_back(std::move(sequence));
        }
    }
    file.close();

    result.total_itemsets = global_itemset_count;
    result.num_sequences = db.size();

    // Build sequence starts
    result.sequence_starts.resize(db.size());
    int pos = 0;
    for (size_t i = 0; i < db.size(); ++i) {
        result.sequence_starts[i] = pos;
        pos += static_cast<int>(db[i].size());
    }

    // Build vertical database: item -> list of (sequence_id, itemset_id_in_sequence)
    int global_pos = 0;
    for (size_t seq_idx = 0; seq_idx < db.size(); ++seq_idx) {
        for (size_t itemset_idx = 0; itemset_idx < db[seq_idx].size(); ++itemset_idx) {
            for (int item : db[seq_idx][itemset_idx]) {
                result.vertical_db[item].emplace_back(
                    static_cast<int>(seq_idx), 
                    static_cast<int>(itemset_idx)
                );
            }
            global_pos++;
        }
    }

    return result;
}

}  // namespace parser
