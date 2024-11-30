#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"
#include "algorithms/md/hymd/preprocessing/column_matches/basic_calculator.h"
#include "algorithms/md/hymd/preprocessing/column_matches/column_match_impl.h"
#include "algorithms/md/hymd/preprocessing/column_matches/single_transformer.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"

namespace python_bindings {
namespace detail {
class StrTransform {
public:
    pybind11::object operator()(std::string const& string) const {
        return pybind11::str(string);
    }
};

// TODO: support no-GIL builds and subinterpreter parallelism
constexpr bool kEnablePythonMultithreading = false;

using PyGenericTypeTransformer =
        algos::hymd::preprocessing::column_matches::SingleTransformer<StrTransform, StrTransform>;

class PyComparerCreator {
    struct Comparer {
        pybind11::object comparison_function_;
        algos::hymd::preprocessing::Similarity min_sim_;

        algos::hymd::preprocessing::Similarity operator()(pybind11::object const& l,
                                                          pybind11::object const& r) {
            auto sim = pybind11::cast<algos::hymd::preprocessing::Similarity>(
                    comparison_function_(l, r));
            if (!(0.0 <= sim && sim <= 1.0))
                throw std::domain_error("Similarity must be in the [0.0, 1.0] range, but is " +
                                        std::to_string(sim));
            return sim < min_sim_ ? algos::hymd::kLowestBound : sim;
        }
    };

    pybind11::object comparison_function_;
    algos::hymd::preprocessing::Similarity min_sim_;

public:
    PyComparerCreator(pybind11::object comparison_function,
                      algos::hymd::preprocessing::Similarity min_sim)
        : comparison_function_(std::move(comparison_function)), min_sim_(min_sim) {}

    auto operator()() const {
        return Comparer{comparison_function_, min_sim_};
    }
};

class PyComparerCreatorSupplier {
    pybind11::object comparison_function_;
    algos::hymd::preprocessing::Similarity min_sim_;

public:
    auto operator()(std::vector<pybind11::object> const*, std::vector<pybind11::object> const*,
                    algos::hymd::indexes::KeyedPositionListIndex const&) const {
        return PyComparerCreator{comparison_function_, min_sim_};
    }

    PyComparerCreatorSupplier(pybind11::object comparison_function,
                              algos::hymd::preprocessing::Similarity min_sim)
        : comparison_function_(std::move(comparison_function)), min_sim_(min_sim) {}
};

template <bool Symmetric, bool EqMax>
using Calculator = algos::hymd::preprocessing::column_matches::BasicCalculator<
        PyComparerCreatorSupplier, Symmetric, EqMax, kEnablePythonMultithreading>;

class PyBasicCalculator {
    bool symmetric_;
    bool equality_max_;
    PyComparerCreatorSupplier creator_supplier_;
    // TODO: make picker interface.
    algos::hymd::preprocessing::ccv_id_pickers::SimilaritiesPicker picker_;

public:
    PyBasicCalculator(PyComparerCreatorSupplier creator_supplier,
                      algos::hymd::preprocessing::ccv_id_pickers::SimilaritiesPicker picker,
                      bool symmetric, bool equality_max)
        : symmetric_(symmetric),
          equality_max_(equality_max),
          creator_supplier_(std::move(creator_supplier)),
          picker_(std::move(picker)) {}

    algos::hymd::indexes::ColumnPairMeasurements Calculate(
            std::vector<pybind11::object> const* left_elements,
            std::vector<pybind11::object> const* right_elements,
            algos::hymd::indexes::KeyedPositionListIndex const& right_pli,
            util::WorkerThreadPool* pool_ptr) const {
        if (symmetric_) {
            if (equality_max_) {
                auto calc = Calculator<true, true>(creator_supplier_, picker_);
                return calc.Calculate(left_elements, right_elements, right_pli, pool_ptr);
            } else {
                auto calc = Calculator<true, false>(creator_supplier_, picker_);
                return calc.Calculate(left_elements, right_elements, right_pli, pool_ptr);
            }
        } else {
            if (equality_max_) {
                auto calc = Calculator<false, true>(creator_supplier_, picker_);
                return calc.Calculate(left_elements, right_elements, right_pli, pool_ptr);
            } else {
                auto calc = Calculator<false, false>(creator_supplier_, picker_);
                return calc.Calculate(left_elements, right_elements, right_pli, pool_ptr);
            }
        }
    }
};

using ObjMeasureBase = algos::hymd::preprocessing::column_matches::ColumnMatchImpl<
        detail::PyGenericTypeTransformer, detail::PyBasicCalculator>;
}  // namespace detail

class PyLhsCCVIDsPicker {
    pybind11::object picker_function_;

public:
    PyLhsCCVIDsPicker(pybind11::object picker_function)
        : picker_function_(std::move(picker_function)) {}

    std::vector<algos::hymd::ColumnClassifierValueId> operator()(
            std::vector<algos::hymd::preprocessing::Similarity> const& similarities) {
        auto lhs_ccv_ids = pybind11::cast<std::vector<algos::hymd::ColumnClassifierValueId>>(
                picker_function_(similarities));
        auto ccv_ids_end = lhs_ccv_ids.end();
        if (std::adjacent_find(lhs_ccv_ids.begin(), ccv_ids_end, std::greater_equal{}) !=
            ccv_ids_end)
            throw std::domain_error("LHS indices must be a strictly increasing sequence.");
        auto first_ccv_id = lhs_ccv_ids.front();
        if (first_ccv_id == algos::hymd::kLowestCCValueId)
            throw std::domain_error("0 must not be used in the LHS index list.");
        auto last_ccv_id = lhs_ccv_ids.back();
        std::size_t const similarities_size = similarities.size();
        if (last_ccv_id >= similarities_size)
            throw std::domain_error("Last LHS index out of range (" + std::to_string(last_ccv_id) +
                                    " >= " + std::to_string(similarities_size) + ")");
        std::vector<algos::hymd::ColumnClassifierValueId> ccv_ids;
        ccv_ids.reserve(lhs_ccv_ids.size() + 1);
        ccv_ids.push_back(algos::hymd::kLowestCCValueId);
        ccv_ids.insert(ccv_ids.end(), lhs_ccv_ids.begin(), lhs_ccv_ids.end());
        return ccv_ids;
    }
};

using ObjMeasureTransformFuncs = detail::PyGenericTypeTransformer::TransformFunctionsOption;

class Custom : public detail::ObjMeasureBase {
public:
    Custom(pybind11::object comparison_function,
           algos::hymd::preprocessing::column_matches::ColumnIdentifier left_column_identifier,
           algos::hymd::preprocessing::column_matches::ColumnIdentifier right_column_identifier,
           ObjMeasureTransformFuncs transform_functions, bool symmetrical, bool equality_is_highest,
           model::md::DecisionBoundary min_sim, std::string name,
           pybind11::object lhs_indices_picker)
        : detail::ObjMeasureBase{
                  symmetrical && equality_is_highest,
                  std::move(name),
                  std::move(left_column_identifier),
                  std::move(right_column_identifier),
                  detail::PyGenericTypeTransformer{std::move(transform_functions)},
                  detail::PyBasicCalculator({std::move(comparison_function), min_sim},
                                            PyLhsCCVIDsPicker{std::move(lhs_indices_picker)},
                                            symmetrical, equality_is_highest)} {}

    Custom(pybind11::object comparison_function,
           algos::hymd::preprocessing::column_matches::ColumnIdentifier left_column_identifier,
           algos::hymd::preprocessing::column_matches::ColumnIdentifier right_column_identifier,
           ObjMeasureTransformFuncs transform_functions, bool symmetrical, bool equality_is_highest,
           model::md::DecisionBoundary min_sim, std::string name, std::size_t size_limit)
        : detail::ObjMeasureBase{
                  symmetrical && equality_is_highest,
                  std::move(name),
                  std::move(left_column_identifier),
                  std::move(right_column_identifier),
                  detail::PyGenericTypeTransformer{std::move(transform_functions)},
                  detail::PyBasicCalculator(
                          {std::move(comparison_function), min_sim},
                          algos::hymd::preprocessing::ccv_id_pickers::IndexUniform<
                                  algos::hymd::preprocessing::Similarity>{size_limit},
                          symmetrical, equality_is_highest)} {}

    Custom(pybind11::object comparison_function,
           algos::hymd::preprocessing::column_matches::ColumnIdentifier left_column_identifier,
           algos::hymd::preprocessing::column_matches::ColumnIdentifier right_column_identifier,
           bool classic_measure, ObjMeasureTransformFuncs transform_functions,
           model::md::DecisionBoundary min_sim, std::string name, std::size_t size_limit)
        : detail::ObjMeasureBase{
                  classic_measure,
                  std::move(name),
                  std::move(left_column_identifier),
                  std::move(right_column_identifier),
                  detail::PyGenericTypeTransformer{std::move(transform_functions)},
                  detail::PyBasicCalculator(
                          {std::move(comparison_function), min_sim},
                          algos::hymd::preprocessing::ccv_id_pickers::IndexUniform<
                                  algos::hymd::preprocessing::Similarity>{size_limit},
                          classic_measure, classic_measure)} {}
};
}  // namespace python_bindings
