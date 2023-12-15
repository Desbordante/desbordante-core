#pragma once

#include "algorithms/ucc/ucc_verifier/ucc_verifier.h"
#include "get_algorithm.h"
#include "py_algorithm.h"

namespace python_bindings {

class PyUCCVerifier : public PyAlgorithm<algos::UCCVerifier, PyAlgorithmBase> {
    using PyAlgorithmBase::algorithm_;

public:
    [[nodiscard]] bool UCCHolds() const {
        return GetAlgorithm<algos::UCCVerifier>(algorithm_).UCCHolds();
    }

    [[nodiscard]] size_t GetNumClustersViolatingUCC() const {
        return GetAlgorithm<algos::UCCVerifier>(algorithm_).GetNumClustersViolatingUCC();
    }

    [[nodiscard]] size_t GetNumRowsViolatingUCC() const {
        return GetAlgorithm<algos::UCCVerifier>(algorithm_).GetNumRowsViolatingUCC();
    }

    [[nodiscard]] std::vector<model::PLI::Cluster> GetClustersViolatingUCC() const {
        return GetAlgorithm<algos::UCCVerifier>(algorithm_).GetClustersViolatingUCC();
    }
};

}  // namespace python_bindings
