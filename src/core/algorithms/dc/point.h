#ifndef POINT_H
#define POINT_H

#include <exception>

#include "component.h"
#include "type.h"

template <class T>
class Point {
private:
    std::vector<T> comps_;  // components of a vector in d-dimensional space

public:
    Point() = default;

    Point(std::vector<T> const& comps) : comps_(comps) {}

    Point(Point const& point) : comps_(point.comps_) {};

    Point(Point&& point) : comps_(std::move(point.comps_)) {};

    size_t get_dim() const {
        return comps_.size();
    }

    Component& operator[](size_t i) {
        if (i >= get_dim()) throw std::out_of_range("Index out of range");
        return comps_[i];
    };

    Component operator[](size_t i) const {
        if (i >= get_dim()) throw std::out_of_range("Index out of range");
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

#endif  // POINT_H