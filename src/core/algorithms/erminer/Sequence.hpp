#pragma once
#include <vector>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <memory>

class Sequence;
class Sequence {
private:
    std::vector<std::vector<int>> itemsets;
    int id;
    
public:
    explicit Sequence(int id);
    
    void addItemset(const std::vector<int>& itemset);
    
    int getId() const;
    
    const std::vector<std::vector<int>>& getItemsets() const;
    const std::vector<int>& get(int index) const;
    
    int size() const;
    
    std::string toString() const;
    
    std::unique_ptr<Sequence> cloneSequenceMinusItems(
        const std::unordered_map<int, std::unordered_set<int>>& mapSequenceID,
        double relativeMinSup) const;
    
    std::unique_ptr<Sequence> cloneSequenceMinusItems(
        double relativeMinSup,
        const std::unordered_map<int, std::unordered_set<Sequence*>>& mapSequenceID) const;
    
private:
    std::vector<int> cloneItemsetMinusItems(
        const std::vector<int>& itemset,
        const std::unordered_map<int, std::unordered_set<int>>& mapSequenceID,
        double minSupportAbsolute) const;
    
    std::vector<int> cloneItemsetMinusItems(
        double relativeMinsup,
        const std::vector<int>& itemset,
        const std::unordered_map<int, std::unordered_set<Sequence*>>& mapSequenceID) const;
};