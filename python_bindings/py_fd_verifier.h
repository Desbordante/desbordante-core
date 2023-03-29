#pragma once

#include "algorithms/fd_verifier/fd_verifier.h"
#include "py_algorithm.h"

namespace python_bindings {

class PyFDVerifier : public PyAlgorithm<algos::fd_verifier::FDVerifier, PyAlgorithmBase> {
    using PyAlgorithm<algos::fd_verifier::FDVerifier, PyAlgorithmBase>::GetAlgorithm;

public:
    [[nodiscard]] bool FDHolds() const {
        return GetAlgorithm().FDHolds();
    }

    [[nodiscard]] size_t GetNumErrorClusters() const {
        return GetAlgorithm().GetNumErrorClusters();
    }

    [[nodiscard]] size_t GetNumErrorRows() const {
        return GetAlgorithm().GetNumErrorRows();
    }

    [[nodiscard]] long double GetError() const {
        return GetAlgorithm().GetError();
    }

    [[nodiscard]] std::vector<algos::fd_verifier::Highlight> const& GetHighlights() const {
        return GetAlgorithm().GetHighlights();
    }
};

}  // namespace python_bindings
