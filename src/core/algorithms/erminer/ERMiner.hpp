#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <memory>
#include <chrono>
#include <filesystem>

#include "algorithms/algorithm.h"
#include "config/option.h"
#include "config/names.h"

#include "SequenceDatabase.hpp"
#include "Occurence.hpp"
#include "LeftRule.hpp"
#include "RightRule.hpp"
#include "LeftEquivalenceClass.hpp"
#include "RightEquivalenceClass.hpp"
#include "ExpandLeftStore.hpp"
#include "SparseMatrix.hpp"

namespace algos {

struct Rule {
    std::vector<int> antecedent;
    std::vector<int> consequent;
    int support;
    double confidence;
    std::string toString() const;
};   


class AlgoERMiner : public Algorithm {
private:
    long timeStart;
    long timeEnd;
    int ruleCount;
    
    double minConfidence;
    int minsuppRelative;
    
    std::unique_ptr<SequenceDatabase> database;
    
    std::unordered_map<int, std::unordered_map<int, Occurence>> mapItemCount;
    std::ofstream writer;
    ExpandLeftStore store;
    SparseMatrix matrix;
    
    int maxAntecedentSize;
    int maxConsequentSize;
    long totalCandidateCount;
    long candidatePrunedCount;

    std::vector<Rule> discoveredRules;
    
    std::string input_path_;
    std::string output_path_;
    double min_support_;
    double min_confidence_;
    
public:
    AlgoERMiner();
    
    void RegisterOptions();
    
    void ResetState() override;
    void LoadDataInternal() override;
    unsigned long long ExecuteInternal() override;
    
    void runAlgorithm(double minSupport, double minConfidence, const std::string& input, const std::string& output);
    void runAlgorithm(const std::string& input, const std::string& output, int relativeMinsup, double minConfidence);
    
    void setMaxAntecedentSize(int size);
    void setMaxConsequentSize(int size);
    
    void printStats() const;
    
    std::vector<Rule> getRules() const;

private:
    void calculateFrequencyOfEachItem();
    void generateMatrix();
    
    void calculateTidsetsIJandJI(
        const std::unordered_map<int, Occurence>& occurencesI,
        const std::unordered_map<int, Occurence>& occurencesJ,
        std::vector<int>& tidsIJ, std::vector<int>& tidsJI) const;
    
    void registerRule11(
        int intI, int intJ,
        const std::vector<int>& tidsI, const std::vector<int>& tidsJ, const std::vector<int>& tidsIJ,
        const std::unordered_map<int, Occurence>& occurencesI, const std::unordered_map<int, Occurence>& occurencesJ,
        std::unordered_map<int, LeftEquivalenceClass>& mapEclassLeft, std::unordered_map<int, RightEquivalenceClass>& mapEclassRight);
        
    void expandLeft(LeftEquivalenceClass& eclass);
    void expandRight(RightEquivalenceClass& eclass);
    void saveRule(const std::vector<int>& tidsIJ, double confIJ, const std::vector<int>& itemsetI, const std::vector<int>& itemsetJ);
    
    std::vector<int> concatenate(const std::vector<int>& itemset, int item) const;
};

}