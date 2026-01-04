#include "location_list.h"

#include <algorithm>
#include <iterator>

namespace algos::maxfem {

std::shared_ptr<LocationList> algos::maxfem::LocationList::Merge(LocationList const& other) const {
    std::vector<size_t> result;
    result.reserve(loc_list_.size() + other.loc_list_.size());
    std::set_union(loc_list_.begin(), loc_list_.end(), other.loc_list_.begin(),
                   other.loc_list_.end(), std::back_inserter(result));
    return std::make_shared<LocationList>(std::move(result));
}

}  // namespace algos::maxfem
