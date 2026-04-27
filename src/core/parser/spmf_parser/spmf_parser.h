#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

namespace parser {

/* Result struct containing parsed SPMF sequence data */
struct SPMFSequenceData {
    /* Item -> Bitmap mapping for vertical database representation */
    std::unordered_map<int, std::vector<std::pair<int, int>>> vertical_db;
    
    /* Sequence starts: for each sequence index i, gives the starting position
     * in the global itemset list */
    std::vector<int> sequence_starts;
    
    /* Total itemsets in all sequences */
    int total_itemsets = 0;
    
    /* Number of sequences */
    int num_sequences = 0;
};

/* SPMF Sequence Parser
 * Parses SPMF format:
 * - Elements separated by spaces
 * - "-1" separates itemsets within a sequence
 * - "-2" marks end of sequence
 * 
 * Example:
 * 1 2 3 -1 4 5 -2
 * This is one sequence with two itemsets: {1,2,3} and {4,5}
 * 
 * Lines starting with #, %, @ are treated as comments
 */
class SPMFParser {
public:
    explicit SPMFParser(std::filesystem::path const& path);
    
    /* Parse the SPMF file and return sequence data */
    SPMFSequenceData Parse(const std::set<int>& must_appear_items = {});
    
private:
    std::filesystem::path path_;
};

}  // namespace parser
