#pragma once

#include "algorithms/fd/model/raw_ucc.h"
#include "algorithms/fd/model/ucc.h"
#include "util/config/indices/type.h"

namespace python_bindings {

class PyUCC {
private:
    util::config::IndicesType ucc_indices_;

public:
    explicit PyUCC(model::RawUCC const& raw_ucc);
    explicit PyUCC(model::UCC const& ucc) : PyUCC(ucc.GetColumnIndices()) {}

    [[nodiscard]] std::string ToString() const;

    [[nodiscard]] util::config::IndicesType const& GetUCC() const noexcept {
        return ucc_indices_;
    }
};

}  // namespace python_bindings
