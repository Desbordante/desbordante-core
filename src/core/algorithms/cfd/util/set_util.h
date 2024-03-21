#pragma once

#include <algorithm>
#include <bitset>
#include <cmath>
#include <concepts>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <ranges>
#include <vector>

// see algorithms/cfd/LICENSE

namespace algos::cfd {

std::vector<int> Range(int, int, int = 1);
std::vector<int> Iota(unsigned);

template <typename T>
bool IsSubsetOf(T const& sub, T const& super) {
    return std::includes(super.begin(), super.end(), sub.begin(), sub.end());
}

template <typename T>
T ConstructSubset(T const& items, typename T::value_type const leave_out) {
    if (items.size() == 1) return T();
    T sub;
    sub.reserve(items.size() - 1);
    for (unsigned mi = 0; mi < items.size(); mi++) {
        if (items[mi] != leave_out) {
            sub.push_back(items[mi]);
        }
    }
    return sub;
}

template <typename T>
void InsertSorted(T const& in, typename T::value_type const item, T& out) {
    auto it = in.begin();
    while (it != in.end() && *it < item) {
        it++;
    }
    if (it != in.begin()) {
        out.insert(out.begin(), in.begin(), it);
    }
    int offset = static_cast<int>(it - in.begin());
    out.insert(out.begin() + offset, item);
    if (in.size() && it != in.end()) {
        out.insert(out.begin() + offset + 1, it, in.end());
    }
}

template <typename T>
T ConstructIntersection(T const& lhs, T const& rhs) {
    T isect(std::min(lhs.size(), rhs.size()));
    auto it = std::set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), isect.begin());
    isect.resize(static_cast<int>(it - isect.begin()));
    return isect;
}

template <typename T>
T SetDiff(T const& lhs, T const& rhs) {
    T res;
    std::set_difference(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                        std::inserter(res, res.begin()));
    return res;
}

template <typename T>
T* Join(T const* lhs, T const* rhs) {
    T* uni = new T(lhs->size() + rhs->size());
    auto it = std::set_union(lhs->begin(), lhs->end(), rhs->begin(), rhs->end(), uni->begin());
    uni->resize(static_cast<int>(it - uni->begin()));
    return uni;
}

template <typename T>
T Join(T const& lhs, T const& rhs) {
    T uni(lhs.size() + rhs.size());
    auto it = std::set_union(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), uni.begin());
    uni.resize(static_cast<int>(it - uni.begin()));
    return uni;
}

template <typename T>
T Join(T const& lhs, typename T::value_type const& rhs) {
    T res;
    res.reserve(lhs.size() + 1);
    InsertSorted(lhs, rhs, res);
    return res;
}

template <typename T>
T* Join(T const* lhs, typename T::value_type const& rhs_item) {
    T* res = new T();
    res->reserve(lhs->size() + 1);
    InsertSorted(*lhs, rhs_item, *res);
    return res;
}

template <typename T>
T Remove(T const& lhs, typename T::value_type const& rhs_item) {
    T res;
    res.reserve(lhs.size() - 1);
    auto const& lower = std::lower_bound(lhs.begin(), lhs.end(), rhs_item);
    res.insert(res.begin(), lhs.begin(), lower);
    res.insert(res.begin() + (lower - lhs.begin()), lower + 1, lhs.end());
    return res;
}

template <typename T>
bool Contains(T const& collection, typename T::value_type const& item) {
    return std::find(collection.begin(), collection.end(), item) != collection.end();
}

template <typename T, typename S>
T ConstructProjection(T const& collection, S const& indices) {
    T res;
    for (int i : indices) {
        res.push_back(collection[i]);
    }
    return res;
}

template <typename T>
std::vector<T> Split(T const& collection, typename T::value_type const& spl) {
    std::vector<T> res(1);
    for (int i : collection) {
        if (i == spl) {
            res.push_back(T());
        } else {
            res.back().push_back(i);
        }
    }
    res.pop_back();
    return res;
}

template <typename T>
T GetMaxElem(std::vector<std::pair<T, int>> const& collection) {
    auto comparator_less = [](std::pair<T, int> a, std::pair<T, int> b) {
        if (a.second < b.second) {
            return true;
        }
        return false;
    };

    auto result = std::max_element(collection.begin(), collection.end(), comparator_less);
    size_t max_element_index = std::distance(collection.begin(), result);
    return collection[max_element_index].first;
}

template <typename T>
void Shuffle(T& collection) {
    std::mt19937 gen(time(nullptr));
    std::shuffle(collection.begin(), collection.end(), gen);
}
}  // namespace algos::cfd
