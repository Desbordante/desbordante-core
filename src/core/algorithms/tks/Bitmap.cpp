#include "Bitmap.hpp"
#include <algorithm>
#include <sstream>
#include <climits>

Bitmap::Bitmap(int lastBitIndex) 
    : numBits(lastBitIndex + 1), cacheValid(true) {
    bits.resize((numBits + 63) / 64, 0);
}

void Bitmap::registerBit(int sid, int tid, const std::vector<int>& sequencesSize) {
    if (sid < 0 || sid >= static_cast<int>(sequencesSize.size())) return;
    int pos = sequencesSize[sid] + tid;
    if (pos < 0 || pos >= numBits) return;
    size_t block = static_cast<size_t>(pos / 64);
    int offset = pos % 64;
    if (block < bits.size()) {
        bits[block] |= (1ULL << offset);
    }
}

int Bitmap::getSupport(const std::vector<int>& sequencesSize) const {
    if (sequencesSize.empty()) return 0;
    

    std::vector<int> extendedSequencesSize = sequencesSize;
    extendedSequencesSize.push_back(numBits);
    
    std::vector<bool> seqHasItem(sequencesSize.size(), false);
    int totalSequences = static_cast<int>(sequencesSize.size());
    
    for (int pos = 0; pos < numBits; ++pos) {
        size_t block = static_cast<size_t>(pos / 64);
        int offset = pos % 64;
        if (block >= bits.size()) continue;
        
        if (bits[block] & (1ULL << offset)) {
            auto it = std::upper_bound(extendedSequencesSize.begin(), 
                                     extendedSequencesSize.end(), pos);
            if (it == extendedSequencesSize.begin()) continue;
            
            int sid = static_cast<int>(std::distance(extendedSequencesSize.begin(), it)) - 1;
            
            if (sid >= 0 && sid < totalSequences) {
                seqHasItem[sid] = true;
            }
        }
    }
    
    int support = 0;
    for (bool b : seqHasItem) if (b) support++;
    return support;
}

int Bitmap::getSupportWithoutGapTotal(const std::vector<int>& sequencesSize) const {
    return getSupport(sequencesSize);
}

Bitmap Bitmap::createNewBitmapSStep(const Bitmap& otherBitmap,
                                   const std::vector<int>& sequencesSize,
                                   int lastBitIndex, int maxGap) const {
    Bitmap newBitmap(lastBitIndex);
    
    if (sequencesSize.empty()) return newBitmap;
    
    // S-STEP: для каждой позиции в otherBitmap, найти все позиции в this
    // которые находятся в одной последовательности, но в БОЛЕЕ РАННЕМ этапе
    
    // Сначала создаём обратную карту: pos -> (sid, tid)
    auto getPosInfo = [&sequencesSize](int pos) -> std::pair<int, int> {
        // Находим sid: это такой индекс что sequencesSize[sid] <= pos < sequencesSize[sid+1]
        for (size_t i = 0; i < sequencesSize.size(); ++i) {
            int seqStart = sequencesSize[i];
            int seqEnd = (i + 1 < sequencesSize.size()) ? sequencesSize[i + 1] : (INT_MAX);
            if (pos >= seqStart && pos < seqEnd) {
                int tid = pos - seqStart;
                return {i, tid};
            }
        }
        return {-1, -1};
    };
    
    for (int pos2 = 0; pos2 <= lastBitIndex; ++pos2) {
        size_t block2 = static_cast<size_t>(pos2 / 64);
        int offset2 = pos2 % 64;
        if (block2 >= otherBitmap.bits.size()) continue;
        if (!(otherBitmap.bits[block2] & (1ULL << offset2))) continue;
        
        auto [sid2, tid2] = getPosInfo(pos2);
        if (sid2 < 0) continue;
        
        for (int pos1 = 0; pos1 <= lastBitIndex; ++pos1) {
            size_t block1 = static_cast<size_t>(pos1 / 64);
            int offset1 = pos1 % 64;
            if (block1 >= bits.size()) continue;
            if (!(bits[block1] & (1ULL << offset1))) continue;
            
            auto [sid1, tid1] = getPosInfo(pos1);
            if (sid1 < 0) continue;
            
            if (sid1 == sid2 && tid1 < tid2 && (tid2 - tid1) <= maxGap) {
                size_t newBlock = static_cast<size_t>(pos2 / 64);
                int newOffset = pos2 % 64;
                if (newBlock < newBitmap.bits.size()) {
                    newBitmap.bits[newBlock] |= (1ULL << newOffset);
                }
                break; 
            }
        }
    }
    
    return newBitmap;
}

Bitmap Bitmap::createNewBitmapIStep(const Bitmap& otherBitmap,
                                   const std::vector<int>& sequencesSize,
                                   int lastBitIndex) const {
    Bitmap newBitmap(lastBitIndex);
    
    if (sequencesSize.empty()) return newBitmap;
    
    // I-STEP: для каждой позиции в otherBitmap, найти позицию в this
    // которая находится в той же последовательности и в ТОМ ЖЕ этапе
    
    auto getPosInfo = [&sequencesSize](int pos) -> std::pair<int, int> {
        for (size_t i = 0; i < sequencesSize.size(); ++i) {
            int seqStart = sequencesSize[i];
            int seqEnd = (i + 1 < sequencesSize.size()) ? sequencesSize[i + 1] : (INT_MAX);
            if (pos >= seqStart && pos < seqEnd) {
                int tid = pos - seqStart;
                return {i, tid};
            }
        }
        return {-1, -1};
    };
    
    for (int pos2 = 0; pos2 <= lastBitIndex; ++pos2) {
        size_t block2 = static_cast<size_t>(pos2 / 64);
        int offset2 = pos2 % 64;
        if (block2 >= otherBitmap.bits.size()) continue;
        if (!(otherBitmap.bits[block2] & (1ULL << offset2))) continue;
        
        auto [sid2, tid2] = getPosInfo(pos2);
        if (sid2 < 0) continue;
        
        for (int pos1 = 0; pos1 <= lastBitIndex; ++pos1) {
            size_t block1 = static_cast<size_t>(pos1 / 64);
            int offset1 = pos1 % 64;
            if (block1 >= bits.size()) continue;
            if (!(bits[block1] & (1ULL << offset1))) continue;
            
            auto [sid1, tid1] = getPosInfo(pos1);
            if (sid1 < 0) continue;
            
            if (sid1 == sid2 && tid1 == tid2) {
                size_t newBlock = static_cast<size_t>(pos2 / 64);
                int newOffset = pos2 % 64;
                if (newBlock < newBitmap.bits.size()) {
                    newBitmap.bits[newBlock] |= (1ULL << newOffset);
                }
                break;
            }
        }
    }
    
    return newBitmap;
}

std::string Bitmap::getSIDs(const std::vector<int>& sequencesSize) const {
    std::ostringstream oss;
    if (sequencesSize.empty()) return oss.str();
    
    std::vector<int> extendedSequencesSize = sequencesSize;
    extendedSequencesSize.push_back(numBits);
    
    std::vector<bool> sids(sequencesSize.size(), false);

    for (int pos = 0; pos < numBits; ++pos) {
        size_t block = static_cast<size_t>(pos / 64);
        int offset = pos % 64;
        if (block >= bits.size()) continue;
        
        if (bits[block] & (1ULL << offset)) {
            auto it = std::upper_bound(extendedSequencesSize.begin(), 
                                     extendedSequencesSize.end(), pos);
            if (it == extendedSequencesSize.begin()) continue;
            
            int sid = static_cast<int>(std::distance(extendedSequencesSize.begin(), it)) - 1;
            if (sid >= 0 && sid < static_cast<int>(sequencesSize.size())) {
                sids[sid] = true;
            }
        }
    }

    bool first = true;
    for (size_t i = 0; i < sids.size(); ++i) {
        if (sids[i]) {
            if (!first) oss << " ";
            oss << i;
            first = false;
        }
    }
    return oss.str();
}