#pragma once

#include "algorithms/fd_verifier/fd_verifier.h"
#include "py_primitive.h"

namespace python_bindings {

class PyFDVerifier : public PyPrimitive<algos::fd_verifier::FDVerifier> {
public:
    [[nodiscard]] bool FDHolds() const {
        return primitive_.FDHolds();
    }

    [[nodiscard]] size_t GetNumErrorClusters() const {
        return primitive_.GetNumErrorClusters();
    }

    [[nodiscard]] size_t GetNumErrorRows() const {
        return primitive_.GetNumErrorRows();
    }

    [[nodiscard]] long double GetError() const {
        return primitive_.GetError();
    }

    [[nodiscard]] std::vector<algos::fd_verifier::Highlight> const& GetHighlights() const {
        return primitive_.GetHighlights();
    }
};

}  // namespace python_bindings
