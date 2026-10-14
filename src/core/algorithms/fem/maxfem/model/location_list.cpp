#include "location_list.h"

#include <algorithm>
#include <iterator>

namespace algos::maxfem {

std::shared_ptr<LocationList> algos::maxfem::LocationList::Merge(LocationList const& other) const {
    std::vector<model::Timestamp> result;
    result.reserve(std::min(loc_list_.size(), other.loc_list_.size()));
    std::set_intersection(loc_list_.begin(), loc_list_.end(), other.loc_list_.begin(),
                          other.loc_list_.end(), std::back_inserter(result));
    if (result.capacity() > 2 * result.size()) result.shrink_to_fit();
    return std::make_shared<LocationList>(std::move(result));
}

}  // namespace algos::maxfem
