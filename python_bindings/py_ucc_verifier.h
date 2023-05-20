#pragma once

#include "algorithms/ucc_verifier/ucc_verifier.h"
#include "get_algorithm.h"
#include "py_algorithm.h"

namespace python_bindings {

class PyUCCVerifier : public PyAlgorithm<algos::ucc_verifier::UCCVerifier, PyAlgorithmBase> {
    using PyAlgorithmBase::algorithm_;

public:
    [[nodiscard]] bool UCCHolds() const {
        return GetAlgorithm<algos::ucc_verifier::UCCVerifier>(algorithm_).UCCHolds();
    }

    [[nodiscard]] size_t GetNumErrorClusters() const {
        return GetAlgorithm<algos::ucc_verifier::UCCVerifier>(algorithm_).GetNumErrorClusters();
    }

    [[nodiscard]] size_t GetNumErrorRows() const {
        return GetAlgorithm<algos::ucc_verifier::UCCVerifier>(algorithm_).GetNumErrorRows();
    }

    [[nodiscard]] std::vector<util::PLI::Cluster> const& GetErrorClusters() const {
        return GetAlgorithm<algos::ucc_verifier::UCCVerifier>(algorithm_).GetErrorClusters();
    }
};

}  // namespace python_bindings
