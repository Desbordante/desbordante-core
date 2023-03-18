#pragma once

#include "algorithms/fd_verifier/fd_verifier.h"
#include "py_primitive.h"

namespace python_bindings {

class PyFDVerifier : public PyPrimitive<algos::fd_verifier::FDVerifier> {
public:
    bool FDHolds() const {
        return primitive_.FDHolds();
    }

    size_t GetNumErrorClusters() const {
        return primitive_.GetNumErrorClusters();
    }

    size_t GetNumErrorRows() const {
        return primitive_.GetNumErrorRows();
    }

    long double GetError() const {
        return primitive_.GetError();
    }

    std::vector<algos::fd_verifier::Highlight> const& GetHighlights() const {
        return primitive_.GetHighlights();
    }
};

}  // namespace python_bindings
