#pragma once

#include <cassert>
#include <utility>
#include <vector>

#include <pybind11/pybind11.h>

#include "algorithms/algebraic_constraints/ranges_collection.h"

namespace python_bindings {

class PyRangesCollection {
public:
    using Ranges = std::vector<std::pair<pybind11::float_, pybind11::float_>>;

private:
    std::pair<size_t, size_t> column_indices_;
    Ranges ranges_;

    static std::vector<std::pair<pybind11::float_, pybind11::float_>> GetPyRanges(
            algos::RangesCollection const& r_coll) {
        std::vector<std::pair<pybind11::float_, pybind11::float_>> res;
        res.reserve(r_coll.ranges.size() / 2);
        assert(r_coll.ranges.size() % 2 == 0);
        for (size_t i = 0; i < r_coll.ranges.size(); i += 2) {
            // TODO: change this once a proper conversion mechanism from `model::INumericType` is
            // implemented
            std::string l_endpoint = r_coll.col_pair.num_type->ValueToString(r_coll.ranges[i]);
            std::string r_endpoint = r_coll.col_pair.num_type->ValueToString(r_coll.ranges[i + 1]);
            res.emplace_back(pybind11::float_(pybind11::str(l_endpoint)),
                             pybind11::float_(pybind11::str(r_endpoint)));
        }
        return res;
    }

public:
    explicit PyRangesCollection(algos::RangesCollection const& r_coll)
        : column_indices_(r_coll.col_pair.col_i), ranges_(GetPyRanges(r_coll)){};

    [[nodiscard]] std::pair<size_t, size_t> GetColumnIndices() const {
        return column_indices_;
    }

    [[nodiscard]] Ranges const& GetRanges() const {
        return ranges_;
    }
};

}  // namespace python_bindings
