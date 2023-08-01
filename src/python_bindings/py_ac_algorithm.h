#pragma once

#include "algorithms/algebraic_constraints/ac_algorithm.h"
#include "algorithms/algebraic_constraints/ac_exception.h"
#include "get_algorithm.h"
#include "py_ac_ranges_collection.h"
#include "py_algorithm.h"

namespace python_bindings {

class PyACAlgorithm : public PyAlgorithm<algos::ACAlgorithm, PyAlgorithmBase> {
    using PyAlgorithmBase::algorithm_;

public:
    [[nodiscard]] std::vector<PyRangesCollection> GetACRanges() const {
        std::vector<algos::RangesCollection> const& collections =
                GetAlgorithm<algos::ACAlgorithm>(algorithm_).GetRangesCollections();
        std::vector<PyRangesCollection> py_collections;
        py_collections.reserve(collections.size());
        for (auto const& c : collections) {
            py_collections.emplace_back(c);
        }
        return py_collections;
    }

    [[nodiscard]] std::vector<algos::ACException> GetACExceptions() const {
        GetAlgorithm<algos::ACAlgorithm>(algorithm_).CollectACExceptions();
        return GetAlgorithm<algos::ACAlgorithm>(algorithm_).GetACExceptions();
    }
};

}  // namespace python_bindings
