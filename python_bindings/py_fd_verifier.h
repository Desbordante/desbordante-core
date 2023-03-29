#pragma once

#include "algorithms/fd_verifier/fd_verifier.h"
#include "get_algorithm.h"
#include "py_algorithm.h"

namespace python_bindings {

class PyFDVerifier : public PyAlgorithm<algos::fd_verifier::FDVerifier, PyAlgorithmBase> {
    using PyAlgorithmBase::algorithm_;

public:
    [[nodiscard]] bool FDHolds() const {
        return GetAlgorithm<algos::fd_verifier::FDVerifier>(algorithm_).FDHolds();
    }

    [[nodiscard]] size_t GetNumErrorClusters() const {
        return GetAlgorithm<algos::fd_verifier::FDVerifier>(algorithm_).GetNumErrorClusters();
    }

    [[nodiscard]] size_t GetNumErrorRows() const {
        return GetAlgorithm<algos::fd_verifier::FDVerifier>(algorithm_).GetNumErrorRows();
    }

    [[nodiscard]] long double GetError() const {
        return GetAlgorithm<algos::fd_verifier::FDVerifier>(algorithm_).GetError();
    }

    [[nodiscard]] std::vector<algos::fd_verifier::Highlight> const& GetHighlights() const {
        return GetAlgorithm<algos::fd_verifier::FDVerifier>(algorithm_).GetHighlights();
    }
};

}  // namespace python_bindings
