#pragma once

#include <exception>

#include "component.h"
#include "type.h"

namespace algos {

namespace dc {

// @brief Point used for query search in kd-tree
template <class T>
class Point {
private:
    std::vector<T> comps_;

public:
    Point() = default;

    Point(std::vector<T> const& comps) : comps_(comps) {};

    Point(std::vector<T>&& comps) : comps_(std::move(comps)) {};

    Point(Point const& point) : comps_(point.comps_) {}

    Point(Point&& point) : comps_(std::move(point.comps_)) {};

    std::string ToString() const {
        std::string res = "";
        for (size_t i = 0; i < comps_.size(); i++) {
            res += comps_[i].ToString();
            if (i != comps_.size() - 1) res += ", ";
        }

        return "(" + res + ")";
    }

    size_t GetDim() const {
        return comps_.size();
    }

    Component& operator[](size_t i) {
        if (i >= GetDim()) throw std::out_of_range("Index out of range");
        return comps_[i];
    };

    Component operator[](size_t i) const {
        if (i >= GetDim()) throw std::out_of_range("Index out of range");
        return comps_[i];
    };

    Point& operator=(Point const& point) {
        comps_ = point.comps_;

        return *this;
    };

    Point& operator=(Point&& point) {
        comps_ = std::move(point.comps_);
        return *this;
    };
};

}  // namespace dc

}  // namespace algos
