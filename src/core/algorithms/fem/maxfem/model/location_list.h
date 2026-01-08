#pragma once

#include <memory>
#include <vector>

#include "core/model/sequence/timestamp.h"

namespace algos::maxfem {

class LocationList {
private:
    // Elements of loc_lists_ are sorted
    std::vector<model::Timestamp> loc_list_;

public:
    LocationList() {}

    LocationList(std::vector<model::Timestamp> loc_list) : loc_list_(std::move(loc_list)) {}

    void PushBack(model::Timestamp new_location) {
        loc_list_.push_back(new_location);
    }

    std::shared_ptr<LocationList> Merge(LocationList const& other) const;

    size_t Size() const {
        return loc_list_.size();
    }

    std::vector<model::Timestamp> const& GetLocationList() const {
        return loc_list_;
    }
};

}  // namespace algos::maxfem
