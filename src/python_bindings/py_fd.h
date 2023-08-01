#pragma once

#include <string>
#include <vector>

#include "algorithms/fd/raw_fd.h"
#include "config/indices/type.h"

namespace python_bindings {

class PyFD {
private:
    config::IndicesType lhs_indices_{};
    config::IndexType rhs_index_;

public:
    explicit PyFD(RawFD const& fd);

    [[nodiscard]] std::string ToString() const;

    [[nodiscard]] config::IndexType GetRhs() const {
        return rhs_index_;
    }

    [[nodiscard]] config::IndicesType const& GetLhs() const noexcept {
        return lhs_indices_;
    }
};

}  // namespace python_bindings
