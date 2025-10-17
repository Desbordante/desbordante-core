#include "algorithms/gfd/gfd_validator/balancer.h"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <map>
#include <numeric>
#include <vector>

std::vector<std::vector<int>> Balancer::Balance(std::vector<int> const& weights,
                                                int const& processors_num) {
    m_ = std::min(processors_num, static_cast<int>(weights.size()));
    result_ = {};
    if (weights.empty()) {
        result_.resize(processors_num);
        return result_;
    }
    for (std::size_t i = 0; i < m_; ++i) {
        // the first value is index
        std::vector<int> temp = {static_cast<int>(i)};
        result_.push_back(temp);
    }
    // fill processors initially
    // count optimal
    optimal_ = 0;
    std::size_t i = 0;
    for (int const& weight : weights) {
        result_.at(i++).push_back(weight);
        i = i == m_ ? 0 : i;
        optimal_ += weight;
    }
    optimal_ /= m_;
    // sort processors (for convenience)
    for (std::vector<int>& processor : result_) {
        std::sort(processor.begin() + 1, processor.end());
    }
    // ALGORITHM
    DeleteLarge();
    Prepare();
    DeleteFirstSmall();
    DeleteSecondSmall();
    FullLarge();
    FullSmall();
    // delete indices
    for (std::vector<int>& processor : result_) {
        processor.erase(processor.begin());
    }
    for (std::size_t i = 0; i < processors_num - m_; ++i) {
        std::vector<int> empty = {};
        result_.push_back(empty);
    }
    return result_;
}

void Balancer::DeleteLarge() {
    deleted_large_ = {};
    for (std::vector<int>& processor : result_) {
        auto border = processor.end();
        for (auto it = --processor.end(); it != processor.begin() + 1; --it) {
            if (*(it - 1) > optimal_ / 2) {
                deleted_large_.push_back(*it);
                border = it;
            } else {
                break;
            }
        }
        processor.erase(border, processor.end());
    }
}

void Balancer::Prepare() {
    for (std::size_t i = 0; i < m_; ++i) {
        quality_.emplace(i, std::tuple<int, int, int>(0, 0, 0));
    }
    for (std::vector<int> const& processor : result_) {
        auto last_small = processor.end();
        auto last = processor.end();
        if (*(--processor.end()) > optimal_ / 2) {
            --last_small;
        }
        if (processor.begin() + 1 == last_small) {
            continue;
        }
        int a = 0;
        int b = 0;
        float sum_small = std::accumulate(processor.begin() + 1, last_small, 0, std::plus<int>());
        float sum = std::accumulate(processor.begin() + 1, last, 0, std::plus<int>());
        while (sum_small > optimal_ / 2) {
            ++a;
            --last_small;
            sum_small -= *last_small;
        }
        while (sum > optimal_) {
            ++b;
            --last;
            sum -= *last;
        }
        std::get<0>(quality_.at(processor.at(0))) = a;
        std::get<1>(quality_.at(processor.at(0))) = b;
        std::get<2>(quality_.at(processor.at(0))) = a - b;
    }
}

void Balancer::DeleteFirstSmall() {
    // sort for convenience
    deleted_small_ = {};
    std::vector<std::vector<int>> small_processors = {};
    std::vector<std::vector<int>> large_processors = {};
    for (std::vector<int> const& processor : result_) {
        if (*(--processor.end()) > optimal_ / 2) {
            large_processors.push_back(processor);
        } else {
            small_processors.push_back(processor);
        }
    }
    auto c_greater = [this](std::vector<int> const& a, std::vector<int> const& b) {
        return std::get<2>(quality_.at(a.at(0))) > std::get<2>(quality_.at(b.at(0)));
    };
    sort(small_processors.begin(), small_processors.end(), c_greater);
    sort(large_processors.begin(), large_processors.end(), c_greater);
    result_.clear();
    result_.insert(result_.end(), small_processors.begin(), small_processors.end());
    result_.insert(result_.end(), large_processors.begin(), large_processors.end());
    large_procs_num_ = large_processors.size();
    std::size_t larges_num = large_processors.size() + deleted_large_.size();
    // work
    border_ = larges_num < m_ ? result_.end() - larges_num : result_.begin();
    for (auto it = border_; it != result_.end(); ++it) {
        auto last = it->end();
        if (*(last - 1) > optimal_ / 2) {
            --last;
        }
        for (auto cur = last - std::get<0>(quality_.at(*it->begin())); cur != last; ++cur) {
            deleted_small_.push_back(*cur);
        }
        it->erase(last - std::get<0>(quality_.at(*it->begin())), last);
    }
}

void Balancer::DeleteSecondSmall() {
    for (auto it = result_.begin(); it != border_; ++it) {
        auto last = it->end();
        for (auto cur = last - std::get<1>(quality_.at(*it->begin())); cur != last; ++cur) {
            deleted_small_.push_back(*cur);
        }
        it->erase(last - std::get<1>(quality_.at(*it->begin())), last);
    }
}

void Balancer::PutWeight(int const& weight) {
    sort(result_.begin(), result_.end(), [](std::vector<int> const& a, std::vector<int> const& b) {
        return std::accumulate(a.begin(), a.end(), 0, std::plus<int>()) <
               std::accumulate(b.begin(), b.end(), 0, std::plus<int>());
    });
    result_.begin()->push_back(weight);
}

void Balancer::FullLarge() {
    std::size_t i = 0;
    for (int const& weight : deleted_large_) {
        if (i < m_ - large_procs_num_) {
            (result_.begin() + i)->push_back(weight);
        } else {
            PutWeight(weight);
        }
        ++i;
    }
}

void Balancer::FullSmall() {
    for (int const& weight : deleted_small_) {
        PutWeight(weight);
    }
}
