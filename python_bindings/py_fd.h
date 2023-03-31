#pragma once

#include <string>
#include <vector>

#include "algorithms/options/indices/type.h"
#include "model/raw_fd.h"

namespace python_bindings {

class PyFD {
private:
    algos::config::IndicesType lhs_indices_{};
    algos::config::IndexType rhs_index_;

public:
    explicit PyFD(RawFD const& fd);

    [[nodiscard]] std::string ToString() const;

    [[nodiscard]] algos::config::IndexType GetRhs() const {
        return rhs_index_;
    }

    [[nodiscard]] algos::config::IndicesType GetLhs() const {
        return lhs_indices_;
    }
};

}  // namespace python_bindings
