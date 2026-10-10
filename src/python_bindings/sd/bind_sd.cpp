#include "python_bindings/sd/bind_sd.h"

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include "core/algorithms/sd/sd_miner/sd_miner.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}

namespace python_bindings {
void BindSD(py::module_& main_module) {
    using namespace algos::sd_miner;

    auto sd_module = main_module.def_submodule("sd");

    py::class_<SDCandidateInterval>(sd_module, "SDCandidateInterval")
            .def_readonly("left", &SDCandidateInterval::left,
                          "Inclusive left position in rows sorted by (lhs, rhs, input order).")
            .def_readonly("right", &SDCandidateInterval::right,
                          "Inclusive right position in rows sorted by (lhs, rhs, input order).")
            .def_readonly("confidence", &SDCandidateInterval::confidence);

    py::class_<SDTableauPattern>(sd_module, "SDTableauPattern")
            .def_readonly("left_position", &SDTableauPattern::left_position,
                          "Inclusive left position in rows sorted by (lhs, rhs, input order).")
            .def_readonly("right_position", &SDTableauPattern::right_position,
                          "Inclusive right position in rows sorted by (lhs, rhs, input order).")
            .def_readonly("left_x", &SDTableauPattern::left_x,
                          "Lhs value at left_position. Use positions to distinguish duplicate lhs "
                          "values.")
            .def_readonly("right_x", &SDTableauPattern::right_x,
                          "Lhs value at right_position. Use positions to distinguish duplicate lhs "
                          "values.")
            .def_readonly("support", &SDTableauPattern::support)
            .def_readonly("confidence", &SDTableauPattern::confidence);

    BindPrimitiveNoBase<SDMiner>(sd_module, "SDMiner")
            .def("get_candidates", &SDMiner::GetCandidates,
                 "Return maximal candidate intervals ordered by their left positions.")
            .def("get_tableau", &SDMiner::GetTableau,
                 "Return tableau patterns ordered by their left positions.")
            .def("get_sorted_row_indices", &SDMiner::GetSortedRowIndices,
                 "Map each sorted position to its zero-based input row index. Slice "
                 "this list by a pattern's inclusive position bounds to obtain SDVerifier "
                 "indices.")
            .def("get_global_support", &SDMiner::GetGlobalSupport);

    main_module.attr("sd") = sd_module;
}
}  // namespace python_bindings
