#include <vector>

#include "raw_fd.h"
#include "types.h"
#include "validator.h"

namespace algos::hyfd {

struct Validator::FDValidations {
    std::vector<RawFD> invalid_fds_;
    IdPairs comparison_suggestions_;

    unsigned count_validations_ = 0;
    unsigned count_intersections_ = 0;

    void add(FDValidations&& other) {
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
