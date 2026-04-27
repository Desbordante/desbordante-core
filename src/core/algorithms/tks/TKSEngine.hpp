#pragma once
#include "Bitmap.hpp"
#include "Prefix.hpp"
#include "PatternTKS.hpp"
#include "Candidate.hpp"
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <stdexcept>

class TKSEngine {
public:
    TKSEngine();

    std::priority_queue<PatternTKS> runAlgorithm(const std::string& input,
                                                 const std::string& outputFilePath,
                                                 int k);

    void setMaximumPatternLength(int maximumPatternLength);
    void setMinimumPatternLength(int minimumPatternLength);
    void setMustAppearItems(const std::vector<int>& mustAppearItems);
    void setMaxGap(int maxGap);
    void showSequenceIdentifiersInOutput(bool showSequenceIdentifiers);
    void printStatistics();
    void writeResultTofile(const std::string& path);

private:
    std::priority_queue<PatternTKS> tks(const std::string& input, int k);
    void save(PatternTKS& pattern);
    void registerAsCandidate(Candidate&& candidate);
    void dfsPruning(Prefix prefix, Bitmap prefixBitmap,
                   const std::vector<int>& sn, const std::vector<int>& in,
                   int hasToBeGreaterThanForIStep, int prefixLength);
    bool itemMustAppearInPatterns(int item) const;

    long startTime;
    long startMiningTime;
    long endTime;
    int minsup;
    int minsupAfterPreProcessing;
    int k;

    std::unordered_map<int, Bitmap> verticalDB;
    std::vector<int> sequencesSize;
    int lastBitIndex;

    std::priority_queue<PatternTKS> kPatterns;
    std::priority_queue<Candidate> candidates;
    std::unordered_set<int> discardedItems;
    std::vector<std::vector<int>> inMemoryDB;

    int maxCandidateCount;
    int candidateExplored;

    bool useDiscardedItemsPruningStrategy = true;
    bool usePruneBranchesInsideDFSPruning = true;
    bool rebuildCandidateTreeWhenTooLarge = false;
    bool useCooccurrenceInformation = false;

    int addedCandidatesSinceLastRebuilt;
    const size_t MIN_CANDIDATES_COUNT_BEFORE_REBUILD = 1500;
    const int MIN_ADDED_CANDIDATE_COUNT_SINCE_LAST_REBUILD_BEFORE_REBUILD = 400;

    std::unordered_map<int, std::unordered_map<int, int>> coocMapAfter;
    std::unordered_map<int, std::unordered_map<int, int>> coocMapEquals;

    int minimumPatternLength;
    int maximumPatternLength;
    std::vector<int> mustAppearItems;
    int maxGap;
    bool outputSequenceIdentifiers;

    bool exactTopKMode = false;

public:
    void setUseCooccurrenceInformation(bool v) { useCooccurrenceInformation = v; }
    void setUseDiscardedItemsPruningStrategy(bool v) { useDiscardedItemsPruningStrategy = v; }
    void setUsePruneBranchesInsideDFSPruning(bool v) { usePruneBranchesInsideDFSPruning = v; }
    void setRebuildCandidateTreeWhenTooLarge(bool v) { rebuildCandidateTreeWhenTooLarge = v; }
    void setExactTopKMode(bool v) { exactTopKMode = v; }

public:
    int getMaxCandidateCount() const { return maxCandidateCount; }
    int getCandidateExplored() const { return candidateExplored; }
    int getKPatternsCount() const { return static_cast<int>(kPatterns.size()); }
    int getMinSupAfterPreprocessing() const { return minsupAfterPreProcessing; }
    int getFinalMinSup() const { return minsup; }
};