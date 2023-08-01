#pragma once

#include "algorithms/ucc/raw_ucc.h"
#include "algorithms/ucc/ucc.h"
#include "config/indices/type.h"

namespace python_bindings {

class PyUCC {
private:
    config::IndicesType ucc_indices_;

public:
    explicit PyUCC(model::RawUCC const& raw_ucc);
    explicit PyUCC(model::UCC const& ucc) : PyUCC(ucc.GetColumnIndices()) {}

    [[nodiscard]] std::string ToString() const;

    [[nodiscard]] config::IndicesType const& GetUCC() const noexcept {
        return ucc_indices_;
    }
};

}  // namespace python_bindings
