#pragma once

#include <vector>

namespace algos::maxfem {

class LocationList {
private:
    std::vector<size_t> loc_list_;
public:
    LocationList(std::vector<size_t> loc_list) : loc_list_(std::move(loc_list)) {}

    void PushBack(size_t new_location) {
        loc_list_.push_back(new_location);
    }

    LocationList Merge(LocationList const& other) const;

    size_t Size() const {
        return loc_list_.size();
    }
};

}  // namespace algos::maxfem
