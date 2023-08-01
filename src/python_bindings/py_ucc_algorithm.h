#pragma once

#include <list>

#include "algorithms/ucc/ucc.h"
#include "algorithms/ucc/ucc_algorithm.h"
#include "get_algorithm.h"
#include "py_algorithm.h"
#include "py_ucc.h"

namespace python_bindings {

class PyUCCAlgorithmBase : public PyAlgorithmBase {
protected:
    using PyAlgorithmBase::PyAlgorithmBase;

public:
    [[nodiscard]] std::vector<PyUCC> GetUCCs() const {
        auto const& ucc_algo = GetAlgorithm<algos::UCCAlgorithm>(algorithm_);
        std::list<model::UCC> const& ucc_list = ucc_algo.UCCList();
        std::vector<PyUCC> py_uccs;
        py_uccs.reserve(ucc_list.size());

        for (model::UCC const& ucc : ucc_list) {
            py_uccs.emplace_back(ucc);
        }

        return py_uccs;
    }
};

template <typename T>
using PyUCCAlgorithm = PyAlgorithm<T, PyUCCAlgorithmBase>;

}  // namespace python_bindings
