#include "non_fds.h"

#include <utility>

#include <boost/dynamic_bitset.hpp>

namespace algos::hyfd {

void NonFds::Add(boost::dynamic_bitset<>&& column_set) {
    if (total_non_fds_.insert(column_set).second) {
        new_non_fds_.Add(std::move(column_set));
    }
}

NonFDList NonFds::MoveOutNewNonFds() {
    size_t num_attributes = NumAttributes();
    NonFDList old = std::move(new_non_fds_);
    new_non_fds_ = NonFDList{num_attributes};
    return old;
}

}  // namespace algos::hyfd
