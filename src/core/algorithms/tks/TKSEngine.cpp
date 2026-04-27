#include "TKSEngine.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <climits>

TKSEngine::TKSEngine()
    : minsup(0), minsupAfterPreProcessing(0), k(0), lastBitIndex(0),
      maxCandidateCount(0), candidateExplored(0), addedCandidatesSinceLastRebuilt(0),
      minimumPatternLength(0), maximumPatternLength(1000), maxGap(INT_MAX),
      outputSequenceIdentifiers(false) {}

bool TKSEngine::itemMustAppearInPatterns(int item) const {
    if (mustAppearItems.empty()) return true;
    return std::binary_search(mustAppearItems.begin(), mustAppearItems.end(), item);
}



void TKSEngine::save(PatternTKS& pattern) {
    if (!mustAppearItems.empty()) {
        std::unordered_set<int> itemsFound;
        bool foundAll = false;
        for (const Itemset& itemset : pattern.prefix.getItemsets()) {
            for (int item : itemset.getItems()) {
                if (itemMustAppearInPatterns(item)) {
                    itemsFound.insert(item);
                    if (itemsFound.size() == mustAppearItems.size()) {
                        foundAll = true;
                        break;
                    }
                }
            }
            if (foundAll) break;
        }
        if (!foundAll) return;
    }

    kPatterns.push(pattern);
}

void TKSEngine::registerAsCandidate(Candidate&& candidate) {
    candidates.push(std::move(candidate));
    addedCandidatesSinceLastRebuilt++;
    if (static_cast<int>(candidates.size()) > maxCandidateCount) {
        maxCandidateCount = static_cast<int>(candidates.size());
    }
}

void TKSEngine::dfsPruning(Prefix prefix, Bitmap prefixBitmap,
                          const std::vector<int>& sn, const std::vector<int>& in,
                          int hasToBeGreaterThanForIStep, int prefixLength) {
    int newCandidatesLength = prefixLength + 1;

    // S-STEPS
    std::vector<int> sTemp;
    std::vector<Bitmap> sTempBitmaps;
    for (int i : sn) {
        if (useDiscardedItemsPruningStrategy &&
            discardedItems.find(i) != discardedItems.end()) {
            continue;
        }
        if (useCooccurrenceInformation) {
            bool shouldPrune = false;
            for (const Itemset& itemset : prefix.getItemsets()) {
                for (int itemX : itemset.getItems()) {
                    auto it = coocMapAfter.find(itemX);
                    if (it == coocMapAfter.end() ||
                        it->second.find(i) == it->second.end() ||
                        it->second.at(i) < minsup) {
                        shouldPrune = true;
                        break;
                    }
                }
                if (shouldPrune) break;
            }
            if (shouldPrune) continue;
        }
        auto it = verticalDB.find(i);
        if (it == verticalDB.end()) continue;
        Bitmap newBitmap = prefixBitmap.createNewBitmapSStep(it->second, sequencesSize, lastBitIndex, maxGap);
        if (newBitmap.getSupportWithoutGapTotal(sequencesSize) >= 1) {
            sTemp.push_back(i);
            sTempBitmaps.push_back(newBitmap);
        }
    }

    for (size_t k = 0; k < sTemp.size(); ++k) {
        Bitmap& newBitmap = sTempBitmaps[k];
        int item = sTemp[k];
        Prefix prefixSStep = prefix.cloneSequence();
        prefixSStep.addItemset(Itemset(item));
        if (newBitmap.getSupport(sequencesSize) >= 1) {
            if (newCandidatesLength >= minimumPatternLength &&
                newCandidatesLength <= maximumPatternLength) {
                PatternTKS pattern(prefixSStep, newBitmap.getSupport(sequencesSize));
                if (outputSequenceIdentifiers) {
                    pattern.bitmap = std::make_unique<Bitmap>(newBitmap);
                }
                save(pattern);
            }
            if (newCandidatesLength + 1 <= maximumPatternLength) {
                std::vector<int> newSN(sTemp.begin(), sTemp.end());
                registerAsCandidate(Candidate(
                    prefixSStep, newBitmap, newSN, newSN, item, newCandidatesLength,
                    newBitmap.getSupport(sequencesSize)));
            }
        }
    }

    // I-STEPS
    std::vector<int> iTemp;
    std::vector<Bitmap> iTempBitmaps;
    for (int i : in) {
        if (i <= hasToBeGreaterThanForIStep) continue;
        if (useDiscardedItemsPruningStrategy &&
            discardedItems.find(i) != discardedItems.end()) {
            continue;
        }
        if (useCooccurrenceInformation) {
            bool shouldPrune = false;
            for (const Itemset& itemset : prefix.getItemsets()) {
                for (int itemX : itemset.getItems()) {
                    auto it = coocMapEquals.find(itemX);
                    if (it == coocMapEquals.end() ||
                        it->second.find(i) == it->second.end() ||
                        it->second.at(i) < minsup) {
                        shouldPrune = true;
                        break;
                    }
                }
                if (shouldPrune) break;
            }
            if (shouldPrune) continue;
        }
        auto it = verticalDB.find(i);
        if (it == verticalDB.end()) continue;
        Bitmap newBitmap = prefixBitmap.createNewBitmapIStep(it->second, sequencesSize, lastBitIndex);
        if (newBitmap.getSupport(sequencesSize) >= 1) {
            iTemp.push_back(i);
            iTempBitmaps.push_back(newBitmap);
        }
    }

    for (size_t k = 0; k < iTemp.size(); ++k) {
        Bitmap& newBitmap = iTempBitmaps[k];
        int item = iTemp[k];
        Prefix prefixIStep = prefix.cloneSequence();
        std::vector<Itemset>& itemsets = prefixIStep.getItemsets();
        Itemset& lastItemset = itemsets.back();
        lastItemset.addItem(item);
        if (newBitmap.getSupport(sequencesSize) >= 1) {
            if (newCandidatesLength >= minimumPatternLength &&
                newCandidatesLength <= maximumPatternLength) {
                PatternTKS pattern(prefixIStep, newBitmap.getSupport(sequencesSize));
                if (outputSequenceIdentifiers) {
                    pattern.bitmap = std::make_unique<Bitmap>(newBitmap);
                }
                save(pattern);
            }
            if (newCandidatesLength + 1 <= maximumPatternLength) {
                std::vector<int> newSN(sTemp.begin(), sTemp.end());
                std::vector<int> newIN(iTemp.begin(), iTemp.end());
                registerAsCandidate(Candidate(
                    prefixIStep, newBitmap, newSN, newIN, item, newCandidatesLength,
                    newBitmap.getSupport(sequencesSize)));
            }
        }
    }
}

std::priority_queue<PatternTKS> TKSEngine::tks(const std::string& input, int k) {
    if (k <= 0) {
        throw std::invalid_argument("Parameter k must be > 0");
    }
    if (minimumPatternLength > maximumPatternLength) {
        throw std::invalid_argument(
            "minimumPatternLength (" + std::to_string(minimumPatternLength) +
            ") cannot be > maximumPatternLength (" + std::to_string(maximumPatternLength) + ")");
    }
    if (maxGap < 0) {
        throw std::invalid_argument("maxGap must be >= 0");
    }
    if (input.empty()) {
        throw std::invalid_argument("Input file path cannot be empty");
    }

    this->k = k;
    bool origUseCooc = useCooccurrenceInformation;
    bool origPruneInside = usePruneBranchesInsideDFSPruning;
    bool origDiscarded = useDiscardedItemsPruningStrategy;
    bool origRebuild = rebuildCandidateTreeWhenTooLarge;
    if (exactTopKMode) {
        useCooccurrenceInformation = false;
        usePruneBranchesInsideDFSPruning = false;
        useDiscardedItemsPruningStrategy = false;
        rebuildCandidateTreeWhenTooLarge = false;
    }
    minsup = 1;
    candidateExplored = 0;
    addedCandidatesSinceLastRebuilt = 0;

    kPatterns = std::priority_queue<PatternTKS>();
    candidates = std::priority_queue<Candidate>();
    discardedItems.clear();
    verticalDB.clear();
    coocMapAfter.clear();
    coocMapEquals.clear();
    inMemoryDB.clear();
    sequencesSize.clear();

    lastBitIndex = 0;
    try {
        std::ifstream file(input);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open input file: " + input);
        }
        
        std::string line;
        int bitIndex = 0;

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#' || line[0] == '%' || line[0] == '@') {
                continue;
            }
            std::istringstream iss(line);
            std::vector<int> transaction;
            std::string token;
            bool containsAMustAppearItem = mustAppearItems.empty();
            sequencesSize.push_back(bitIndex);
            
            int separatorCount = 0;

            while (iss >> token) {
                int item = std::stoi(token);
                transaction.push_back(item);
                if (item == -1) {
                    separatorCount++;
                } else if (item == -2) {
                    break;
                } else if (itemMustAppearInPatterns(item)) {
                    containsAMustAppearItem = true;
                }
            }
            
            int numStages = separatorCount + 1;
            bitIndex += numStages;
            
            if (containsAMustAppearItem) {
                inMemoryDB.push_back(transaction);
            }
        }
        lastBitIndex = bitIndex - 1;
        file.close();
        
        if (inMemoryDB.empty()) {
            throw std::runtime_error("Input database is empty or contains no matching sequences");
        }
        

    } catch (const std::exception& e) {
        std::cerr << "Error reading input file: " << e.what() << std::endl;
        throw;
    }

    auto start = std::chrono::system_clock::now();
    startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        start.time_since_epoch()).count();

    int sid = 0;
    for (const auto& transaction : inMemoryDB) {
        int tid = 0;
        for (size_t i = 0; i < transaction.size(); ++i) {
            int item = transaction[i];
            if (item == -1) {
                tid++;
            } else if (item == -2) {
                break;
            } else {
                auto it = verticalDB.find(item);
                if (it == verticalDB.end()) {
                    auto emplace_result = verticalDB.emplace(item, Bitmap(lastBitIndex));
                    it = emplace_result.first;
                }
                it->second.registerBit(sid, tid, sequencesSize);
            }
        }
        sid++; 
    }

    std::vector<int> frequentItems;
        for (auto& entry : verticalDB) {
            int item = entry.first;
            Bitmap& bitmap = entry.second;
            if (bitmap.getSupport(sequencesSize) >= minsup) {
                candidateExplored++;
                Prefix prefix;
                prefix.addItemset(Itemset(item));
                PatternTKS pattern(prefix, bitmap.getSupport(sequencesSize));
                if (outputSequenceIdentifiers) {
                    pattern.bitmap = std::make_unique<Bitmap>(bitmap);
                }
                if (1 >= minimumPatternLength && 1 <= maximumPatternLength) {
                    save(pattern);
                }
                frequentItems.push_back(item);
            }
        }

    if (maximumPatternLength > 1 && useCooccurrenceInformation) {
        coocMapEquals.clear();
        coocMapAfter.clear();

        for (const auto& transaction : inMemoryDB) {
            std::unordered_set<int> alreadyProcessed;
            std::unordered_map<int, std::unordered_set<int>> equalProcessed;

            for (size_t i = 0; i < transaction.size(); ++i) {
                int itemI = transaction[i];
                
                if (itemI < 0) {
                    continue;
                }

                auto& equalSet = equalProcessed[itemI];

                auto bitmapIt = verticalDB.find(itemI);
                if (bitmapIt == verticalDB.end() || 
                    bitmapIt->second.getSupport(sequencesSize) < minsup) {
                    continue;
                }

                std::unordered_set<int> alreadyProcessedB;
                bool sameItemset = true;

                for (size_t j = i + 1; j < transaction.size(); ++j) {
                    int itemJ = transaction[j];
                    
                    if (itemJ < 0) {
                        sameItemset = false;
                        continue;
                    }

                    auto bitmapJIt = verticalDB.find(itemJ);
                    if (bitmapJIt == verticalDB.end() || 
                        bitmapJIt->second.getSupport(sequencesSize) < minsup) {
                        continue;
                    }

                    if (sameItemset) {
                        if (equalSet.find(itemJ) == equalSet.end()) {
                            coocMapEquals[itemI][itemJ]++;
                            equalSet.insert(itemJ);
                        }
                    } else {
                        if (alreadyProcessedB.find(itemJ) == alreadyProcessedB.end()) {
                            coocMapAfter[itemI][itemJ]++;
                            alreadyProcessedB.insert(itemJ);
                        }
                    }
                }
                alreadyProcessed.insert(itemI);
            }
        }
    }

    minsupAfterPreProcessing = minsup;

    if (maximumPatternLength > 1) {
        if (useCooccurrenceInformation) {
            for (auto& entry : verticalDB) {
                int item = entry.first;
                Bitmap& bitmap = entry.second;
                if (bitmap.getSupport(sequencesSize) >= minsup) {
                    candidateExplored++;
                    Prefix prefix;
                    prefix.addItemset(Itemset(item));
                    std::vector<int> afterItems;
                    auto coocIt = coocMapAfter.find(item);
                    if (coocIt != coocMapAfter.end()) {
                        for (const auto& pair : coocIt->second) {
                            afterItems.push_back(pair.first);
                        }
                    }
                    int support = bitmap.getSupport(sequencesSize);
                    registerAsCandidate(Candidate(
                        prefix, bitmap, afterItems, afterItems, item, 1, support));
                }
            }
        } else {
            for (int item : frequentItems) {
                auto it = verticalDB.find(item);
                if (it == verticalDB.end()) continue;
                if (it->second.getSupport(sequencesSize) >= minsup) {
                    candidateExplored++;
                    Prefix prefix;
                    prefix.addItemset(Itemset(item));
                    std::vector<int> freqItemsVec(frequentItems.begin(), frequentItems.end());
                    registerAsCandidate(Candidate(
                    prefix, it->second, freqItemsVec, freqItemsVec, item, 1,
                    it->second.getSupport(sequencesSize)));
                }
            }
        }

        auto miningStart = std::chrono::system_clock::now();
        startMiningTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            miningStart.time_since_epoch()).count();

        while (!candidates.empty()) {
            Candidate cand = candidates.top();
            candidates.pop();

            if (cand.bitmap.getSupport(sequencesSize) < minsup) {
                break;
            }

            candidateExplored++;
            dfsPruning(cand.prefix, cand.bitmap, cand.sn, cand.in,
                      cand.hasToBeGreaterThanForIStep, cand.candidateLength);

            if (rebuildCandidateTreeWhenTooLarge &&
                candidates.size() > MIN_CANDIDATES_COUNT_BEFORE_REBUILD &&
                addedCandidatesSinceLastRebuilt > MIN_ADDED_CANDIDATE_COUNT_SINCE_LAST_REBUILD_BEFORE_REBUILD) {
                std::priority_queue<Candidate> temp;
                while (!candidates.empty()) {
                    Candidate c = candidates.top();
                    candidates.pop();
                    if (c.bitmap.getSupport(sequencesSize) >= minsup) {
                        temp.push(c);
                    }
                }
                candidates = std::move(temp);
                addedCandidatesSinceLastRebuilt = 0;
            }
        }
    }

    auto end = std::chrono::system_clock::now();
    endTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        end.time_since_epoch()).count();

    if (exactTopKMode) {
        useCooccurrenceInformation = origUseCooc;
        usePruneBranchesInsideDFSPruning = origPruneInside;
        useDiscardedItemsPruningStrategy = origDiscarded;
        rebuildCandidateTreeWhenTooLarge = origRebuild;
    }

    return kPatterns;
}

std::priority_queue<PatternTKS> TKSEngine::runAlgorithm(const std::string& input,
                                                       const std::string& outputFilePath,
                                                       int k) {
    const int internalK = 1000000;
    tks(input, internalK);
    
    std::vector<PatternTKS> allPatterns;
    while (!kPatterns.empty()) {
        allPatterns.push_back(std::move(const_cast<PatternTKS&>(kPatterns.top())));
        kPatterns.pop();
    }
    
    int take = std::min(static_cast<int>(allPatterns.size()), k);
    for (int i = 0; i < take; ++i) {
        kPatterns.push(std::move(allPatterns[i]));
    }
    
    writeResultTofile(outputFilePath);
    return kPatterns;
}

void TKSEngine::writeResultTofile(const std::string& path) {
    if (path.empty()) {
        throw std::invalid_argument("Output file path cannot be empty");
    }

    try {
        std::ofstream writer(path);
        if (!writer.is_open()) {
            throw std::runtime_error("Cannot open output file: " + path);
        }
        
        std::vector<PatternTKS> patterns;
        while (!kPatterns.empty()) {
            patterns.push_back(std::move(const_cast<PatternTKS&>(kPatterns.top())));
            kPatterns.pop();
        }
        
        for (auto it = patterns.rbegin(); it != patterns.rend(); ++it) {
            writer << it->prefix.toString() << " #SUP: " << it->support;
            if (outputSequenceIdentifiers && it->bitmap) {
                writer << " #SID: " << it->bitmap->getSIDs(sequencesSize);
            }
            writer << std::endl;
        }
        
        for (auto& pattern : patterns) {
            kPatterns.push(std::move(pattern));
        }
        
        writer.close();
    } catch (const std::exception& e) {
        std::cerr << "Error writing output file: " << e.what() << std::endl;
        throw;
    }
}

void TKSEngine::printStatistics() {
    std::cout << "=============  Algorithm TKS v0.97 - STATISTICS =============" << std::endl;
    std::cout << "Minsup after preprocessing: " << minsupAfterPreProcessing << std::endl;
    std::cout << "Max candidates: " << maxCandidateCount << std::endl;
    std::cout << "Candidates explored: " << candidateExplored << std::endl;
    std::cout << "Pattern found count: " << kPatterns.size() << std::endl;
    std::cout << "Time preprocessing: " << (startMiningTime - startTime) << " ms" << std::endl;
    std::cout << "Total time: " << (endTime - startTime) << " ms" << std::endl;
    std::cout << "Final minsup value: " << minsup << std::endl;
    std::cout << "===================================================" << std::endl;
}

void TKSEngine::setMaximumPatternLength(int maxLen) {
    if (maxLen <= 0) {
        throw std::invalid_argument("Maximum pattern length must be > 0");
    }
    maximumPatternLength = maxLen;
}

void TKSEngine::setMinimumPatternLength(int minLen) {
    if (minLen < 0) {
        throw std::invalid_argument("Minimum pattern length must be >= 0");
    }
    minimumPatternLength = minLen;
}

void TKSEngine::setMustAppearItems(const std::vector<int>& items) {
    if (!items.empty()) {
        for (int item : items) {
            if (item <= 0) {
                throw std::invalid_argument("Items must be positive integers");
            }
        }
    }
    mustAppearItems = items;
    std::sort(mustAppearItems.begin(), mustAppearItems.end());
}

void TKSEngine::setMaxGap(int gap) {
    if (gap < 0) {
        throw std::invalid_argument("Max gap must be >= 0");
    }
    maxGap = gap;
}

void TKSEngine::showSequenceIdentifiersInOutput(bool show) {
    outputSequenceIdentifiers = show;
}