#include "Parser.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

std::unordered_map<Item, Bitmap>
parseSPMF(const std::string& filename,
          std::vector<int>& outSeqStarts,
          int& outTotalItemsets,
          const std::set<Item>& mustAppearItems) {

    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::vector<std::vector<std::vector<Item>>> db;
    outSeqStarts.clear();
    std::string line;
    int globalItemsetCount = 0;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '%' || line[0] == '@') {
            continue;
        }

        std::istringstream iss(line);
        std::string token;
        std::vector<std::vector<Item>> sequence;
        std::vector<Item> currentItemset;
        bool hasMustItem = mustAppearItems.empty();

        while (iss >> token) {
            int val = std::stoi(token);
            if (val == -1) {
                if (!currentItemset.empty()) {
                    sequence.emplace_back(std::move(currentItemset));
                    currentItemset.clear();
                    globalItemsetCount++;
                }
            } else if (val == -2) {
                break;
            } else {
                currentItemset.push_back(val);
                if (!hasMustItem && mustAppearItems.count(val)) {
                    hasMustItem = true;
                }
            }
        }
        if (!currentItemset.empty()) {
            sequence.emplace_back(std::move(currentItemset));
            globalItemsetCount++;
        }

        if (hasMustItem && !sequence.empty()) {
            db.push_back(std::move(sequence));
        }
    }
    file.close();

    outTotalItemsets = globalItemsetCount;
    outSeqStarts.resize(db.size());
    int pos = 0;
    for (size_t i = 0; i < db.size(); ++i) {
        outSeqStarts[i] = pos;
        pos += static_cast<int>(db[i].size());
    }

    std::unordered_map<Item, Bitmap> verticalDB;
    int globalPos = 0;
for (const auto& seq : db) {
    for (const auto& itemset : seq) {
        for (Item item : itemset) {
            // Прямое конструирование без operator[]
            auto it = verticalDB.find(item);
            if (it == verticalDB.end()) {
                // Создаём новый Bitmap и вставляем его
                auto emplace_result = verticalDB.emplace(item, Bitmap(outTotalItemsets - 1));
                it = emplace_result.first;
            }
            it->second.registerBit(static_cast<int>(&seq - &db[0]), 
                                  static_cast<int>(&itemset - &seq[0]), 
                                  outSeqStarts);
        }
        globalPos++;
    }
}

    return verticalDB;
}