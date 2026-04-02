#include "ERMiner.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <cmath>
#include <sstream>
#include <cfloat>
#include <iterator>
#include <vector>

#define DESBORDANTE_OPTION_USING \
    using config::names::kMinimumSupport; \
    using config::names::kMinimumConfidence; \
    using config::names::kTable;

namespace algos {
    

AlgoERMiner::AlgoERMiner() : 
    Algorithm({}),
    timeStart(0),
    timeEnd(0),
    ruleCount(0),
    minConfidence(0),
    minsuppRelative(0),
    database(std::make_unique<SequenceDatabase>()),
    maxAntecedentSize(INT_MAX),
    maxConsequentSize(INT_MAX),
    totalCandidateCount(0),
    candidatePrunedCount(0),
    min_support_(0),
    min_confidence_(0) {
    
    RegisterOptions();
    MakeOptionsAvailable({config::names::kTable, config::names::kMinimumSupport,
                          config::names::kMinimumConfidence});
}

void AlgoERMiner::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    
    auto check_minsup = [](double val) {
        if (val <= 0 || val > 1) {
            throw config::ConfigurationError(
                    "Minimum support must be a value between 0 (exclusive) and 1 (inclusive).");
        }
    };
    
    auto check_minconf = [](double val) {
        if (val <= 0 || val > 1) {
            throw config::ConfigurationError(
                    "Minimum confidence must be a value between 0 (exclusive) and 1 (inclusive).");
        }
    };
    
    RegisterOption(config::Option<std::string>{&input_path_, config::names::kTable, 
                    "Path to input file with sequences", std::string{}});
    
    RegisterOption(config::Option<double>{&min_support_, config::names::kMinimumSupport, 
                    "Minimum support threshold", 0.0}.SetValueCheck(check_minsup));
    
    RegisterOption(config::Option<double>{&min_confidence_, config::names::kMinimumConfidence, 
                    "Minimum confidence threshold", 0.0}.SetValueCheck(check_minconf));
    
    RegisterOption(config::Option<std::string>{&output_path_, "output", 
                    "Path to output file", std::string{}});
}

void AlgoERMiner::LoadDataInternal() {
    database->loadFile(input_path_);
}

void AlgoERMiner::ResetState() {
    ruleCount = 0;
    totalCandidateCount = 0;
    candidatePrunedCount = 0;
    mapItemCount.clear();
    database.reset(new SequenceDatabase());
    matrix = SparseMatrix();
    store = ExpandLeftStore();
    discoveredRules.clear();
}

unsigned long long AlgoERMiner::ExecuteInternal() {
    auto start = std::chrono::high_resolution_clock::now();
    
    runAlgorithm(min_support_, min_confidence_, input_path_, output_path_);
    
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

std::string Rule::toString() const {
    std::stringstream ss;
    
    ss << "[";
    for (size_t i = 0; i < antecedent.size(); i++) {
        ss << antecedent[i];
        if (i != antecedent.size() - 1) {
            ss << ", ";
        }
    }
    
    ss << "] ==> [";
    
    for (size_t i = 0; i < consequent.size(); i++) {
        ss << consequent[i];
        if (i != consequent.size() - 1) {
            ss << ", ";
        }
    }
    
    ss << "]  (support: " << support << ", confidence: " << confidence << ")";
    
    return ss.str();
}


void AlgoERMiner::runAlgorithm(double minSupport, double minConfidence, const std::string& input, const std::string& output) {
    try {
        database = std::make_unique<SequenceDatabase>();
        database->loadFile(input);
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading database: " << e.what() << std::endl;
        return;
    }
    
    this->minsuppRelative = static_cast<int>(std::ceil(minSupport * database->size()));
    runAlgorithm(input, output, minsuppRelative, minConfidence);
}

void AlgoERMiner::runAlgorithm(const std::string& input, const std::string& output, int relativeMinsup, double minConfidence) {
    this->minConfidence = minConfidence;
    ruleCount = 0;
    
    if (!database) {
        try {
            database = std::make_unique<SequenceDatabase>();
            database->loadFile(input);
        } catch (const std::exception& e) {
            std::cerr << "Error loading database: " << e.what() << std::endl;
            return;
        }
    }
    
    
    writer.open(output);
    if (!writer.is_open()) {
        std::cerr << "Cannot open output file: " << output << std::endl;
        return;
    }
    
    this->minsuppRelative = relativeMinsup;
    if (this->minsuppRelative == 0) {
        this->minsuppRelative = 1;
    }
    
    timeStart = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (maxAntecedentSize > 0 && maxConsequentSize > 0) {
        calculateFrequencyOfEachItem();
        generateMatrix();
    }
    
    std::unordered_map<int, LeftEquivalenceClass> mapEclassLeft;
    std::unordered_map<int, RightEquivalenceClass> mapEclassRight;
    
    for (const auto& entry : matrix.matrix) {
        int intI = entry.first;
        const auto& occurencesI = mapItemCount[intI];
        
        if (occurencesI.size() < static_cast<size_t>(minsuppRelative)) {
            continue;
        }
        
        std::vector<int> tidsI;
        for (const auto& occ : occurencesI) {
            tidsI.push_back(occ.first);
        }
        std::sort(tidsI.begin(), tidsI.end());
        
        for (const auto& entryJ : entry.second) {
            int intJ = entryJ.first;
            int supportIJ = entryJ.second;
            
            if (supportIJ < minsuppRelative) {
                continue;
            }
            
            const auto& occurencesJ = mapItemCount[intJ];
            
            if (occurencesJ.size() < static_cast<size_t>(minsuppRelative)) {
                continue;
            }
            
            std::vector<int> tidsJ;
            for (const auto& occ : occurencesJ) {
                tidsJ.push_back(occ.first);
            }
            std::sort(tidsJ.begin(), tidsJ.end());
            
            std::vector<int> tidsIJ;
            std::vector<int> tidsJI;

            calculateTidsetsIJandJI(occurencesI, occurencesJ, tidsIJ, tidsJI);
            
            if (tidsIJ.size() >= static_cast<size_t>(minsuppRelative)) {
                double confIJ = static_cast<double>(tidsIJ.size()) / occurencesI.size();
                std::vector<int> itemsetI = {intI};
                std::vector<int> itemsetJ = {intJ};
                
                if (confIJ >= minConfidence) {
                    saveRule(tidsIJ, confIJ, itemsetI, itemsetJ);
                }
                
                if (maxAntecedentSize > 1 || maxConsequentSize > 1) {
                    registerRule11(intI, intJ, tidsI, tidsJ, tidsIJ, 
                                  occurencesI, occurencesJ, 
                                  mapEclassLeft, mapEclassRight);
                }
            }
            
            if (tidsJI.size() >= static_cast<size_t>(minsuppRelative)) {
                double confJI = static_cast<double>(tidsJI.size()) / occurencesJ.size();
                std::vector<int> itemsetI = {intI};
                std::vector<int> itemsetJ = {intJ};
                
                if (confJI >= minConfidence) {
                    saveRule(tidsJI, confJI, itemsetJ, itemsetI);
                }
                
                if (maxAntecedentSize > 1 || maxConsequentSize > 1) {
                    registerRule11(intJ, intI, tidsJ, tidsI, tidsJI, occurencesJ, occurencesI, mapEclassLeft, mapEclassRight);
                }
            }
        }
    }
    
    if (maxAntecedentSize > 1) {
        for (auto& eclassLeftPair : mapEclassLeft) {
            auto& eclassLeft = eclassLeftPair.second;
            if (eclassLeft.rules.size() > 1) {
                std::vector<LeftRule> rulesVec(eclassLeft.rules.begin(), eclassLeft.rules.end());
                std::sort(rulesVec.begin(), rulesVec.end(),
                    [](const LeftRule& a, const LeftRule& b) {
                        return a.itemsetI[0] < b.itemsetI[0];
                    });
                
                eclassLeft.rules.assign(rulesVec.begin(), rulesVec.end());
                expandLeft(eclassLeft);
            }
        }
    }
    
    if (maxConsequentSize > 1) {
        for (auto& eclassRightPair : mapEclassRight) {
            auto& eclassRight = eclassRightPair.second;
            if (eclassRight.rules.size() > 1) {
                std::vector<RightRule> rulesVec(eclassRight.rules.begin(), eclassRight.rules.end());
                std::sort(rulesVec.begin(), rulesVec.end(),
                    [](const RightRule& a, const RightRule& b) {
                        return a.itemsetJ[0] < b.itemsetJ[0];
                    });
                
                eclassRight.rules.assign(rulesVec.begin(), rulesVec.end());
                expandRight(eclassRight);
            }
        }
    }
    
    for (auto& sizeMap : store.store) {
        for (auto& hashListPair : sizeMap.second) {
            for (auto& eclass : hashListPair.second) {
                if (eclass.rules.size() > 1) {
                    std::vector<LeftRule> rulesVec(eclass.rules.begin(), eclass.rules.end());
                    std::sort(rulesVec.begin(), rulesVec.end(),
                        [](const LeftRule& a, const LeftRule& b) {
                            return a.itemsetI.back() < b.itemsetI.back();
                        });
                    
                    eclass.rules.assign(rulesVec.begin(), rulesVec.end());
                    expandLeft(eclass);
                }
            }
        }
    }
    
    timeEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    writer.close();
    database.reset();
}

void AlgoERMiner::calculateTidsetsIJandJI(
    const std::unordered_map<int, Occurence>& occurencesI,
    const std::unordered_map<int, Occurence>& occurencesJ,
    std::vector<int>& tidsIJ,
    std::vector<int>& tidsJI) const {
    
    std::vector<int> commonTids;
    if (occurencesI.size() < occurencesJ.size()) {
        for (const auto& entryOccI : occurencesI) {
            int tid = entryOccI.first;
            if (occurencesJ.find(tid) != occurencesJ.end()) {
                commonTids.push_back(tid);
            }
        }
    } else {
        for (const auto& entryOccJ : occurencesJ) {
            int tid = entryOccJ.first;
            if (occurencesI.find(tid) != occurencesI.end()) {
                commonTids.push_back(tid);
            }
        }
    }
    
    for (int tid : commonTids) {
        const Occurence& occI = occurencesI.at(tid);
        const Occurence& occJ = occurencesJ.at(tid);
        
        if (occI.firstItemset < occJ.lastItemset) {
            tidsIJ.push_back(tid);
        }
        if (occJ.firstItemset < occI.lastItemset) {
            tidsJI.push_back(tid);
        }
    }
    
    std::sort(tidsIJ.begin(), tidsIJ.end());
    tidsIJ.erase(std::unique(tidsIJ.begin(), tidsIJ.end()), tidsIJ.end());
    
    std::sort(tidsJI.begin(), tidsJI.end());
    tidsJI.erase(std::unique(tidsJI.begin(), tidsJI.end()), tidsJI.end());
}

void AlgoERMiner::calculateFrequencyOfEachItem() {
    mapItemCount.clear();
    
    for (size_t k = 0; k < database->getSequences().size(); k++) {
        const auto& sequence = database->getSequences()[k];
        
        for (int j = 0; j < sequence->size(); j++) {
            const auto& itemset = sequence->get(j);
            
            for (int itemI : itemset) {
                auto& occurences = mapItemCount[itemI];
                auto it = occurences.find(k);
                if (it == occurences.end()) {
                    Occurence occ(static_cast<int16_t>(j), static_cast<int16_t>(j));
                    occurences.emplace(k, occ);
                } else {
                    it->second.lastItemset = static_cast<int16_t>(j);
                }
            }
        }
    }
}

void AlgoERMiner::registerRule11(
    int intI, int intJ,
    const std::vector<int>& tidsI,
    const std::vector<int>& tidsJ,
    const std::vector<int>& tidsIJ,
    const std::unordered_map<int, Occurence>& occurencesI,
    const std::unordered_map<int, Occurence>& occurencesJ,
    std::unordered_map<int, LeftEquivalenceClass>& mapEclassLeft,
    std::unordered_map<int, RightEquivalenceClass>& mapEclassRight) {
    
    auto leftIt = mapEclassLeft.find(intJ);
    if (leftIt == mapEclassLeft.end()) {
        mapEclassLeft.emplace(intJ, LeftEquivalenceClass({intJ}, tidsJ, occurencesJ));
        leftIt = mapEclassLeft.find(intJ);
    }
    leftIt->second.rules.emplace_back(std::vector<int>{intI}, tidsI, tidsIJ);
    
    auto rightIt = mapEclassRight.find(intI);
    if (rightIt == mapEclassRight.end()) {
        mapEclassRight.emplace(intI, RightEquivalenceClass({intI}, tidsI, occurencesI));
        rightIt = mapEclassRight.find(intI);
    }
    rightIt->second.rules.emplace_back(std::vector<int>{intJ}, tidsJ, tidsIJ, occurencesJ);
}

std::vector<int> AlgoERMiner::concatenate(const std::vector<int>& itemset, int item) const {
    std::vector<int> newItemset = itemset;
    newItemset.push_back(item);
    return newItemset;
}

void AlgoERMiner::expandLeft(LeftEquivalenceClass& eclass) {
    std::vector<LeftRule> rulesVec(eclass.rules.begin(), eclass.rules.end());
    
    for (size_t w = 0; w < rulesVec.size() - 1; w++) {
        LeftRule& rule1 = rulesVec[w];
        int d = rule1.itemsetI.back();
        
        LeftEquivalenceClass rulesForRecursion(eclass.itemsetJ, eclass.tidsJ, eclass.occurencesJ);

        for (size_t m = w + 1; m < rulesVec.size(); m++) {
            LeftRule& rule2 = rulesVec[m];
            int c = rule2.itemsetI.back();
            
            if (matrix.getCount(c, d) < minsuppRelative) {
                candidatePrunedCount++;
                totalCandidateCount++;
                continue;
            }
            totalCandidateCount++;
            
            std::vector<int> tidsIC;
            const auto& mapC = mapItemCount[c];
            
            const std::vector<int>& sortedTidsI = rule1.tidsI;
            
            if (sortedTidsI.size() < mapC.size()) {
                int remains = static_cast<int>(sortedTidsI.size());
                for (int tid : sortedTidsI) {
                    if (mapC.find(tid) != mapC.end()) {
                        tidsIC.push_back(tid);
                    }
                    remains--;
                    if (tidsIC.size() + static_cast<size_t>(remains) < static_cast<size_t>(minsuppRelative)) {
                        break;
                    }
                }
            } else {
                int remains = static_cast<int>(mapC.size());
                for (const auto& entry : mapC) {
                    int tid = entry.first;
                    if (std::binary_search(sortedTidsI.begin(), sortedTidsI.end(), tid)) {
                        tidsIC.push_back(tid);
                    }
                    remains--;
                    if (tidsIC.size() + static_cast<size_t>(remains) < static_cast<size_t>(minsuppRelative)) {
                        break;
                    }
                }
            }
            
            std::sort(tidsIC.begin(), tidsIC.end());
            tidsIC.erase(std::unique(tidsIC.begin(), tidsIC.end()), tidsIC.end());
            
            std::vector<int> tidsIC_J;
            const std::vector<int>& sortedTidsIJ = rule1.tidsIJ;
            
            if (sortedTidsIJ.size() < mapC.size()) {
                for (int tid : sortedTidsIJ) {
                    auto occurenceCIt = mapC.find(tid);
                    if (occurenceCIt != mapC.end()) {
                        auto occurenceJIt = eclass.occurencesJ.find(tid);
                        if (occurenceJIt != eclass.occurencesJ.end()) {
                            if (occurenceCIt->second.firstItemset < occurenceJIt->second.lastItemset) {
                                tidsIC_J.push_back(tid);
                            }
                        }
                    }
                }
            } else {
                for (const auto& entryC : mapC) {
                    int tid = entryC.first;
                    if (std::binary_search(sortedTidsIJ.begin(), sortedTidsIJ.end(), tid)) {
                        auto occurenceJIt = eclass.occurencesJ.find(tid);
                        if (occurenceJIt != eclass.occurencesJ.end()) {
                            if (entryC.second.firstItemset < occurenceJIt->second.lastItemset) {
                                tidsIC_J.push_back(tid);
                            }
                        }
                    }
                }
            }
            
            std::sort(tidsIC_J.begin(), tidsIC_J.end());
            tidsIC_J.erase(std::unique(tidsIC_J.begin(), tidsIC_J.end()), tidsIC_J.end());
            
            if (tidsIC_J.size() >= static_cast<size_t>(minsuppRelative)) {
                double confIC_J = static_cast<double>(tidsIC_J.size()) / tidsIC.size();
                std::vector<int> itemsetIC = concatenate(rule1.itemsetI, c);
                
                if (confIC_J >= minConfidence) {
                    saveRule(tidsIC_J, confIC_J, itemsetIC, eclass.itemsetJ);
                }
                
                if (static_cast<int>(itemsetIC.size()) < maxAntecedentSize) {
                    rulesForRecursion.rules.emplace_back(itemsetIC, tidsIC, tidsIC_J);
                }
            }
        }
        
        if (rulesForRecursion.rules.size() > 1) {
            expandLeft(rulesForRecursion);
        }
    }
    
}

void AlgoERMiner::expandRight(RightEquivalenceClass& eclass) {
    std::vector<RightRule> rulesVec(eclass.rules.begin(), eclass.rules.end());
    
    for (size_t w = 0; w < rulesVec.size() - 1; w++) {
        RightRule& rule1 = rulesVec[w];
        int d = rule1.itemsetJ.back();
        
        RightEquivalenceClass rulesForRecursion(eclass.itemsetI, eclass.tidsI, eclass.occurencesI);

        for (size_t m = w + 1; m < rulesVec.size(); m++) {
            RightRule& rule2 = rulesVec[m];
            int c = rule2.itemsetJ.back();
            
            if (matrix.getCount(c, d) < minsuppRelative) {
                candidatePrunedCount++;
                totalCandidateCount++;
                continue;
            }
            totalCandidateCount++;
            
            std::vector<int> tidsI_JC;
            const auto& mapC = mapItemCount[c];
            
            const std::vector<int>& sortedTidsIJ = rule1.tidsIJ;
            
            if (sortedTidsIJ.size() < mapC.size()) {
                int remains = static_cast<int>(sortedTidsIJ.size());
                for (int tid : sortedTidsIJ) {
                    auto occurenceCIt = mapC.find(tid);
                    if (occurenceCIt != mapC.end()) {
                        auto occurenceIIt = eclass.occurencesI.find(tid);
                        if (occurenceIIt != eclass.occurencesI.end()) {
                            if (occurenceCIt->second.lastItemset > occurenceIIt->second.firstItemset) {
                                tidsI_JC.push_back(tid);
                            }
                        }
                    }
                    remains--;
                    if (tidsI_JC.size() + static_cast<size_t>(remains) < static_cast<size_t>(minsuppRelative)) {
                        break;
                    }
                }
            } else {
                int remains = static_cast<int>(mapC.size());
                for (const auto& entryC : mapC) {
                    int tid = entryC.first;
                    if (std::binary_search(sortedTidsIJ.begin(), sortedTidsIJ.end(), tid)) {
                        auto occurenceIIt = eclass.occurencesI.find(tid);
                        if (occurenceIIt != eclass.occurencesI.end()) {
                            if (entryC.second.lastItemset > occurenceIIt->second.firstItemset) {
                                tidsI_JC.push_back(tid);
                            }
                        }
                    }
                    remains--;
                    if (tidsI_JC.size() + static_cast<size_t>(remains) < static_cast<size_t>(minsuppRelative)) {
                        break;
                    }
                }
            }
            
            std::sort(tidsI_JC.begin(), tidsI_JC.end());
            tidsI_JC.erase(std::unique(tidsI_JC.begin(), tidsI_JC.end()), tidsI_JC.end());
            
            if (tidsI_JC.size() >= static_cast<size_t>(minsuppRelative)) {
                std::vector<int> tidsJC;
                std::unordered_map<int, Occurence> occurencesJC;
                
                const std::vector<int>& sortedTidsJ = rule1.tidsJ;
                
                if (sortedTidsJ.size() < mapC.size()) {
                    for (int tid : sortedTidsJ) {
                        auto occurrenceCIt = mapC.find(tid);
                        if (occurrenceCIt != mapC.end()) {
                            tidsJC.push_back(tid);
                            auto occurenceJIt = rule1.occurencesJ.find(tid);
                            if (occurenceJIt != rule1.occurencesJ.end()) {
                                if (occurrenceCIt->second.lastItemset < occurenceJIt->second.lastItemset) {
                                    occurencesJC[tid] = occurrenceCIt->second;
                                } else {
                                    occurencesJC[tid] = occurenceJIt->second;
                                }
                            }
                        }
                    }
                } else {
                    for (const auto& entryC : mapC) {
                        int tid = entryC.first;
                        if (std::binary_search(sortedTidsJ.begin(), sortedTidsJ.end(), tid)) {
                            tidsJC.push_back(tid);
                            auto occurenceJIt = rule1.occurencesJ.find(tid);
                            if (occurenceJIt != rule1.occurencesJ.end()) {
                                if (entryC.second.lastItemset < occurenceJIt->second.lastItemset) {
                                    occurencesJC[tid] = entryC.second;
                                } else {
                                    occurencesJC[tid] = occurenceJIt->second;
                                }
                            }
                        }
                    }
                }
                
                std::sort(tidsJC.begin(), tidsJC.end());
                tidsJC.erase(std::unique(tidsJC.begin(), tidsJC.end()), tidsJC.end());
                
                double confI_JC = static_cast<double>(tidsI_JC.size()) / eclass.tidsI.size();
                std::vector<int> itemsetJC = concatenate(rule1.itemsetJ, c);
                
                if (confI_JC >= minConfidence) {
                    saveRule(tidsI_JC, confI_JC, eclass.itemsetI, itemsetJC);
                }
                
                RightRule rightRule(itemsetJC, tidsJC, tidsI_JC, occurencesJC);
                
                if (static_cast<int>(itemsetJC.size()) < maxConsequentSize) {
                    rulesForRecursion.rules.push_back(rightRule);
                }
                
                if (static_cast<int>(eclass.itemsetI.size()) < maxAntecedentSize) {
                    LeftRule leftRule(eclass.itemsetI, eclass.tidsI, tidsI_JC);
                    store.registerRule(leftRule, itemsetJC, tidsJC, occurencesJC);
                }
            }
        }
        
        if (rulesForRecursion.rules.size() > 1) {
            expandRight(rulesForRecursion);
        }
    }
    
}

void AlgoERMiner::generateMatrix() {
    for (const auto& sequencePtr : database->getSequences()) {
        const auto& sequence = *sequencePtr;
        std::unordered_set<int> alreadyProcessed;
        
        for (const auto& itemsetj : sequence.getItemsets()) {
            for (int itemk : itemsetj) {
                if (alreadyProcessed.find(itemk) != alreadyProcessed.end() || 
                    mapItemCount[itemk].size() < static_cast<size_t>(minsuppRelative)) {
                    continue;
                }
                
                std::unordered_set<int> alreadyProcessedWithRespectToK;
                for (const auto& itemsetjj : sequence.getItemsets()) {
                    for (int itemkk : itemsetjj) {
                        if (itemkk == itemk ||
                            alreadyProcessedWithRespectToK.find(itemkk) != alreadyProcessedWithRespectToK.end() ||
                            mapItemCount[itemkk].size() < static_cast<size_t>(minsuppRelative)) {
                            continue;
                        }
                        
                        matrix.increaseCountOfPair(itemk, itemkk);
                        alreadyProcessedWithRespectToK.insert(itemkk);
                    }
                }
                alreadyProcessed.insert(itemk);
            }
        }
    }
}

void AlgoERMiner::saveRule(const std::vector<int>& tidsIJ, double confIJ, const std::vector<int>& itemsetI, const std::vector<int>& itemsetJ) {
    ruleCount++;

    Rule rule;
    rule.antecedent = itemsetI;
    rule.consequent = itemsetJ;
    rule.support = tidsIJ.size();
    rule.confidence = confIJ;
    discoveredRules.push_back(rule);
    
    std::stringstream buffer;
    for (size_t i = 0; i < itemsetI.size(); i++) {
        buffer << itemsetI[i];
        if (i != itemsetI.size() - 1) {
            buffer << ",";
        }
    }
    
    buffer << " ==> ";
    
    for (size_t i = 0; i < itemsetJ.size(); i++) {
        buffer << itemsetJ[i];
        if (i != itemsetJ.size() - 1) {
            buffer << ",";
        }
    }
    
    buffer << " #SUP: " << tidsIJ.size();
    buffer << " #CONF: " << confIJ;
    
    try {
        writer << buffer.str() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error writing rule: " << e.what() << std::endl;
    }
}

std::vector<Rule> AlgoERMiner::getRules() const {
    return discoveredRules;
}

void AlgoERMiner::setMaxAntecedentSize(int size) {
    maxAntecedentSize = size;
}

void AlgoERMiner::setMaxConsequentSize(int size) {
    maxConsequentSize = size;
}

void AlgoERMiner::printStats() const {
    std::cout << "=============  ERMiner - STATS ========" << std::endl;
    std::cout << "Sequential rules count: " << ruleCount << std::endl;
    std::cout << "Total time: " << (timeEnd - timeStart) << " ms" << std::endl;
    std::cout << "Candidates pruned: " << candidatePrunedCount << " of " << totalCandidateCount << std::endl;
    std::cout << "==========================================" << std::endl;
}
} // namespace algos