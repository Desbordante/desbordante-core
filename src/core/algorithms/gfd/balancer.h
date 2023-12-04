#pragma once
#include <map>
#include <vector>

class Balancer {
private:
    std::size_t m_;
    double optimal_;
    std::vector<std::vector<int>>::iterator border_;
    std::vector<std::vector<int>> result_;
    std::vector<int> deleted_large_ = {};
    std::vector<int> deleted_small_ = {};
    std::size_t large_procs_num_;
    std::map<int, std::tuple<int, int, int>> quality_;

    void DeleteLarge();
    void Prepare();
    void DeleteFirstSmall();
    void DeleteSecondSmall();
    void PutWeight(int const& weight);
    void FullLarge();
    void FullSmall();

public:
    std::vector<std::vector<int>> Balance(std::vector<int> const& weights,
                                          int const& processors_num);
};
