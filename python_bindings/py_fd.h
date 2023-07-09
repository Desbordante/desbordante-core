#pragma once

#include <string>
#include <vector>

#include "algorithms/fd/model/raw_fd.h"
#include "util/config/indices/type.h"

namespace python_bindings {

class PyFD {
private:
    util::config::IndicesType lhs_indices_{};
    util::config::IndexType rhs_index_;

public:
    explicit PyFD(RawFD const& fd);

    [[nodiscard]] std::string ToString() const;

    [[nodiscard]] util::config::IndexType GetRhs() const {
        return rhs_index_;
    }

    [[nodiscard]] util::config::IndicesType const& GetLhs() const noexcept {
        return lhs_indices_;
    }
};

}  // namespace python_bindings
