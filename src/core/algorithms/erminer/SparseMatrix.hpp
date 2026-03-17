#pragma once
#include <unordered_map>

class SparseMatrix {
public:
    std::unordered_map<int, std::unordered_map<int, int>> matrix;
    
    SparseMatrix() = default;

    void increaseCountOfPair(int i, int j);
    
    int getCount(int i, int j) const;
    
    const std::unordered_map<int, std::unordered_map<int, int>>& getMatrix() const;
};