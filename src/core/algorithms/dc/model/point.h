#pragma once

#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/functional/hash/hash.hpp>

#include "core/algorithms/dc/model/component.h"

namespace algos::dc {

// @brief Point used for query search in kd-tree
template <class T>
class Point {
private:
    std::vector<T> comps_;
    size_t index_;

public:
    Point() = default;
    Point(Point const&) = default;
    Point& operator=(Point const& point) = default;
    Point& operator=(Point&& point) = default;

    template <typename U>
    Point(U&& comps, size_t index) : comps_(std::forward<U>(comps)), index_(index) {}

    std::string ToString() const {
        if (comps_.empty()) return {};

        std::stringstream ss;
        ss << '(' << comps_.front().ToString();
        for (auto comp = std::next(comps_.begin()); comp != comps_.end(); ++comp) {
            ss << ", " << comp->ToString();
        }
        ss << ')';

        return ss.str();
    }

    size_t GetDim() const noexcept {
        return comps_.size();
    }

    size_t GetIndex() const noexcept {
        return index_;
    }

    Component& operator[](size_t i) {
        if (i >= GetDim()) throw std::out_of_range("Index out of range");
        return comps_[i];
    };

    Component operator[](size_t i) const {
        if (i >= GetDim()) throw std::out_of_range("Index out of range");
        return comps_[i];
    };

    bool operator==(Point const& rhs) const {
        return comps_ == rhs.comps_;
    }

    class Hasher {
    public:
        size_t operator()(Point const& p) const {
            auto hasher = typename T::Hasher();
            std::vector<T> values = p.comps_;
            std::vector<size_t> res(values.size());
            std::transform(values.begin(), values.end(), res.begin(), hasher);
            res.push_back(p.index_);

            return boost::hash_value(res);
        }
    };
};

}  // namespace algos::dc
