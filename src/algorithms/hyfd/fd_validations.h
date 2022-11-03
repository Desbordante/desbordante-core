#include <vector>

#include "elements/raw_fd.h"
#include "types.h"

namespace algos::hyfd {

struct Validator::FDValidations {
    std::vector<RawFD> invalid_fds_;
    IdPairs comparison_suggestions_;

    int count_validations_ = 0;
    int count_intersections_ = 0;

    void add(FDValidations&& other) noexcept {
        invalid_fds_.insert(invalid_fds_.end(), other.invalid_fds_.begin(),
                            other.invalid_fds_.end());

        comparison_suggestions_.insert(comparison_suggestions_.end(),
                                       other.comparison_suggestions_.begin(),
                                       other.comparison_suggestions_.end());

        count_intersections_ += other.count_intersections_;
        count_validations_ += other.count_validations_;
    }
};

}  // namespace algos::hyfd
