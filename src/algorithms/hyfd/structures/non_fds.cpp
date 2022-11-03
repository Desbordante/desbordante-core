#include "non_fds.h"

namespace algos::hyfd {

void NonFds::Add(boost::dynamic_bitset<>&& column_set) {
    assert(total_non_fds_.size() > column_set.count());

    auto& level = total_non_fds_[column_set.count()];
    if (level.find(column_set) != level.end()) {
        return;
    }

    level.insert(column_set);
    new_non_fds_.Add(std::move(column_set));
}

NonFDList NonFds::MoveOutNewNonFds() {
    NonFDList old = std::move(new_non_fds_);
    new_non_fds_ = NonFDList{total_non_fds_.size()};
    return old;
}

}  // namespace algos::hyfd