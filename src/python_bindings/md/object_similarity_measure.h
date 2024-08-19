#pragma once

#include <stdexcept>
#include <string>

#include <pybind11/pybind11.h>

#include "algorithms/md/hymd/lowest_bound.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/basic_calculator.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/column_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/single_transformer.h"

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
        algos::hymd::preprocessing::similarity_measure::SingleTransformer<StrTransform,
                                                                          StrTransform>;

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

template <bool kSymmetric, bool kEqMax>
using Calculator = algos::hymd::preprocessing::similarity_measure::BasicCalculator<
        PyComparerCreatorSupplier, kSymmetric, kEqMax, kEnablePythonMultithreading>;

class PyBasicCalculator {
    bool symmetric_;
    bool equality_max_;
    PyComparerCreatorSupplier creator_supplier_;
    // TODO: make picker interface.
    algos::hymd::preprocessing::ccv_id_pickers::IndexUniform picker_;

public:
    PyBasicCalculator(PyComparerCreatorSupplier creator_supplier,
                      algos::hymd::preprocessing::ccv_id_pickers::IndexUniform picker,
                      bool symmetric, bool equality_max)
        : symmetric_(symmetric),
          equality_max_(equality_max),
          creator_supplier_(std::move(creator_supplier)),
          picker_(std::move(picker)) {}

    algos::hymd::indexes::SimilarityMeasureOutput Calculate(
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

using ObjMeasureBase = algos::hymd::preprocessing::similarity_measure::ColumnSimilarityMeasure<
        detail::PyGenericTypeTransformer, detail::PyBasicCalculator>;
}  // namespace detail

using ObjMeasureTransformFuncs = detail::PyGenericTypeTransformer::TransformFunctionsOption;

class ObjectSimilarityMeasure : public detail::ObjMeasureBase {
public:
    ObjectSimilarityMeasure(
            pybind11::object comparison_function,
            algos::hymd::preprocessing::similarity_measure::ColumnIdentifier left_column_identifier,
            algos::hymd::preprocessing::similarity_measure::ColumnIdentifier
                    right_column_identifier,
            ObjMeasureTransformFuncs transform_functions, bool symmetrical,
            bool equality_is_highest, model::md::DecisionBoundary min_sim, std::string name,
            std::size_t size_limit)
        : detail::ObjMeasureBase{
                  symmetrical && equality_is_highest,
                  std::move(name),
                  std::move(left_column_identifier),
                  std::move(right_column_identifier),
                  detail::PyGenericTypeTransformer{std::move(transform_functions)},
                  detail::PyBasicCalculator({std::move(comparison_function), min_sim}, {size_limit},
                                            symmetrical, equality_is_highest)} {}

    ObjectSimilarityMeasure(
            pybind11::object comparison_function,
            algos::hymd::preprocessing::similarity_measure::ColumnIdentifier left_column_identifier,
            algos::hymd::preprocessing::similarity_measure::ColumnIdentifier
                    right_column_identifier,
            bool classic_measure, ObjMeasureTransformFuncs transform_functions,
            model::md::DecisionBoundary min_sim, std::string name, std::size_t size_limit)
        : detail::ObjMeasureBase{
                  classic_measure,
                  std::move(name),
                  std::move(left_column_identifier),
                  std::move(right_column_identifier),
                  detail::PyGenericTypeTransformer{std::move(transform_functions)},
                  detail::PyBasicCalculator({std::move(comparison_function), min_sim}, {size_limit},
                                            classic_measure, classic_measure)} {}
};
}  // namespace python_bindings
