#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

class Bitmap {
private:
    std::vector<uint64_t> bits;
    int numBits;
    
    // Кэширование результатов поддержки
    mutable std::unordered_map<size_t, int> supportCache;
    mutable bool cacheValid;

public:
    explicit Bitmap(int lastBitIndex);
    Bitmap(const Bitmap& other) = default;
    Bitmap& operator=(const Bitmap& other) = default;

    void registerBit(int sid, int tid, const std::vector<int>& sequencesSize);
    int getSupport(const std::vector<int>& sequencesSize) const;
    int getSupportWithoutGapTotal(const std::vector<int>& sequencesSize) const;
    Bitmap createNewBitmapSStep(const Bitmap& otherBitmap,
                               const std::vector<int>& sequencesSize,
                               int lastBitIndex, int maxGap) const;
    Bitmap createNewBitmapIStep(const Bitmap& otherBitmap,
                               const std::vector<int>& sequencesSize,
                               int lastBitIndex) const;
    std::string getSIDs(const std::vector<int>& sequencesSize) const;
    
    void clearCache() const { 
        supportCache.clear(); 
        cacheValid = true;
    }
};