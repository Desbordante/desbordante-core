# pragma once

#include <string>

namespace algos::fastod {

class DataAndIndex {
private:
    std::pair<int, int> pair_;
    
public:
    DataAndIndex(int data, int index) noexcept;
    std::string ToString() const noexcept;
    friend bool operator<(DataAndIndex const& x, DataAndIndex const& y);
};

} //namespace algos::fastod
