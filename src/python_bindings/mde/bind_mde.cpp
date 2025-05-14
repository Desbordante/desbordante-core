#include "mde/bind_mde.h"

#include <cstddef>
#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/mde/decision_boundaries/decision_boundary.h"
#include "algorithms/mde/decision_boundaries/similarity.h"
#include "algorithms/mde/hymde/compact_mde_storage.h"
#include "algorithms/mde/hymde/hymde.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/levenshtein_similarity.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/predicate.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/predicate_inverse.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/similarity_ge.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/similarity_le.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"
#include "algorithms/mde/hymde/record_match_indexes/partitioning_functions/attribute_value.h"
#include "algorithms/mde/hymde/record_match_indexes/partitioning_functions/partitioning_function.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/disable.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/index_uniform.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/max_only.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/selector.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_selectors/size_based_selector.h"
#include "algorithms/mde/mde.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
using namespace algos::hymde;
using namespace record_match_indexes::calculators;
using namespace record_match_indexes::partitioning_functions;
using namespace record_match_indexes::rcv_id_selectors;
using namespace record_match_indexes::orders;
using namespace model::mde;
using namespace decision_boundaries;

template <typename BoundType>
auto BindDecisionBoundary(auto&& decision_boundaries_module, auto&& name) {
    auto cls = py::class_<BoundType, decision_boundaries::DecisionBoundary,
                          std::shared_ptr<BoundType>>(decision_boundaries_module, name)
                       .def_property_readonly("value", &BoundType::GetValue);
    return cls;
}

template <typename PartFuncType>
auto BindPartFunc(auto&& module, auto&& name) {
    using CreatorType = PartFuncType::Creator;

    auto cls = py::class_<CreatorType,
                          typename PartitioningFunction<typename PartFuncType::Type>::Creator,
                          std::shared_ptr<CreatorType>>(module, name);
    return cls;
}

template <typename OrderType>
auto BindOrder(auto&& module, auto&& name) {
    auto cls =
            py::class_<OrderType, TotalOrder<typename OrderType::Type>, std::shared_ptr<OrderType>>(
                    module, name)
                    .def(py::init<>());
    return cls;
}

template <typename SelectorType>
auto BindSizeLhsSelector(auto&& module, auto&& name) {
    auto cls = py::class_<SelectorType, SizeBasedSelector, std::shared_ptr<SelectorType>>(module,
                                                                                          name);
    if constexpr (std::is_constructible_v<SelectorType>) {
        cls.def(py::init<>());
    }
    return cls;
}

template <typename CalculatorType>
auto BindCalculatorCreator(auto&& calculator_specifiers, auto&& name) {
    using CreatorType = CalculatorType::Creator;
    auto cls =
            py::class_<CreatorType, Calculator::Creator, std::shared_ptr<CreatorType>>(
                    calculator_specifiers, name)
                    .def(py::init<typename CreatorType::PartitioningFunctionCreatorsOption,
                                  typename CreatorType::OrderPtr, typename CreatorType::SelectorPtr,
                                  typename CreatorType::ComparisonResult>())
                    .def(py::init<typename CreatorType::PartitioningFunctionCreatorsOption,
                                  typename CreatorType::OrderPtr,
                                  std::shared_ptr<SizeBasedSelector const>,
                                  typename CreatorType::ComparisonResult>());
    return cls;
}
}  // namespace

namespace python_bindings {
void BindMde(py::module_& main_module) {
    using namespace py::literals;

    auto mde_module = main_module.def_submodule("mde");

    // MDE
    auto decision_boundaries_module = mde_module.def_submodule("decision_boundaries");
    py::class_<DecisionBoundary, std::shared_ptr<DecisionBoundary>>(decision_boundaries_module,
                                                                    "DecisionBoundary")
            .def("__str__", &DecisionBoundary::ToString);
    BindDecisionBoundary<Similarity>(decision_boundaries_module, "Similarity");

    py::class_<RecordMatch>(mde_module, "RecordMatch")
            .def_property_readonly("left_partitioning_function",
                                   &RecordMatch::GetLeftPartitioningFunction)
            .def_property_readonly("right_partitioning_function",
                                   &RecordMatch::GetRightPartitioningFunction)
            .def_property_readonly("comparison_function", &RecordMatch::GetComparisonFunction)
            .def_property_readonly("order", &RecordMatch::GetOrder)
            .def("to_tuple", &RecordMatch::ToTuple)
            .def("__str__", &RecordMatch::ToString);

    py::class_<RecordClassifier>(mde_module, "RecordClassifier")
            .def_property_readonly("record_match", &RecordClassifier::GetRecordMatch)
            .def_property_readonly("decision_boundary", &RecordClassifier::GetDecisionBoundary)
            .def("to_tuple", &RecordClassifier::ToTuple)
            .def("__str__", &RecordClassifier::ToString);

    py::class_<MDE>(mde_module, "MDE")
            .def_property_readonly("left_table", &MDE::GetLeftTable)
            .def_property_readonly("right_table", &MDE::GetRightTable)
            .def_property_readonly("lhs", &MDE::GetLhs)
            .def_property_readonly("rhs", &MDE::GetRhs)
            .def("__str__", &MDE::ToString);

    // partitioning functions
    // TODO: split into modules by type
    auto partitioning_functions = mde_module.def_submodule("partitioning_functions");

    auto string_pf = partitioning_functions.def_submodule("string");
    py::class_<PartitioningFunction<std::string>::Creator,
               std::shared_ptr<PartitioningFunction<std::string>::Creator>>(string_pf,
                                                                            "PartitioningFunction");
    BindPartFunc<AttributeValue<std::string>>(string_pf, "AttributeValue")
            .def(py::init<AttributeValue<std::string>::Creator::ColumnIdentifier>());

    auto object_pf = partitioning_functions.def_submodule("object_");
    py::class_<PartitioningFunction<py::object>>(object_pf, "PartitioningFunction");
    // TODO: py::object

    // orders
    auto orders = mde_module.def_submodule("orders");

    auto double_ord = orders.def_submodule("double");
    py::class_<TotalOrder<double>, std::shared_ptr<TotalOrder<double>>>(double_ord,
                                                                        "BoundedTotalOrder");
    BindOrder<SimilarityLe>(double_ord, "SimilarityLe");
    BindOrder<SimilarityGe>(double_ord, "SimilarityGe");

    auto object_ord = orders.def_submodule("object_");
    py::class_<TotalOrder<py::object>, std::shared_ptr<TotalOrder<py::object>>>(
            object_ord, "BoundedTotalOrder");
    // TODO

    auto bool_ord = orders.def_submodule("bool_");
    py::class_<TotalOrder<bool>, std::shared_ptr<TotalOrder<bool>>>(bool_ord, "BoundedTotalOrder");
    BindOrder<Predicate>(bool_ord, "Predicate");
    BindOrder<PredicateInverse>(bool_ord, "PredicateInverse");

    // lhs_selectors
    auto lhs_selectors = mde_module.def_submodule("lhs_selectors");

    auto size_selectors = lhs_selectors.def_submodule("size");
    py::class_<SizeBasedSelector, std::shared_ptr<SizeBasedSelector>>(size_selectors, "Selector");
    BindSizeLhsSelector<Disable>(size_selectors, "Disable");
    BindSizeLhsSelector<MaxOnly>(size_selectors, "MaxOnly");
    BindSizeLhsSelector<IndexUniform>(size_selectors, "IndexUniform").def(py::init<std::size_t>());

    auto double_selectors = lhs_selectors.def_submodule("float_");
    py::class_<Selector<double>, std::shared_ptr<Selector<double>>>(double_selectors, "Selector");
    // TODO:

    // calculator creators
    auto calculator_specifiers = mde_module.def_submodule("calculator_specifiers");

    using Creator = Calculator::Creator;
    py::class_<Creator, std::shared_ptr<Creator>>(calculator_specifiers, "CalculatorSpecifier");
    BindCalculatorCreator<LevenshteinSimilarity>(calculator_specifiers, "LevenshteinSimilarity");

    // MDE storage
    py::class_<RecordClassifierSpecification>(mde_module, "RecordClassifierSpecification")
            .def_readonly("record_match_index", &RecordClassifierSpecification::record_match_index)
            .def_readonly("rcv_id", &RecordClassifierSpecification::rcv_id)
            .def("to_tuple", &RecordClassifierSpecification::ToTuple);

    py::class_<LhsSpecification>(mde_module, "LhsSpecification")
            .def_readonly("lhs", &LhsSpecification::lhs)
            .def_readonly("support", &LhsSpecification::support);

    py::class_<SameLhsMDEsSpecification>(mde_module, "SameLhsMDEsSpecification")
            .def_readonly("lhs_spec", &SameLhsMDEsSpecification::lhs_spec)
            .def_readonly("rhss", &SameLhsMDEsSpecification::rhss);

    py::class_<SearchSpaceFactorSpecification>(mde_module, "SearchSpaceFactorSpecification")
            .def_readonly("record_match", &SearchSpaceFactorSpecification::record_match)
            .def_readonly("decision_boundaries",
                          &SearchSpaceFactorSpecification::decision_boundaries);

    py::class_<CompactMDEStorage>(mde_module, "Storage")
            .def_property_readonly("mdes", &CompactMDEStorage::GetAll)
            .def_property_readonly("lhs_info", &CompactMDEStorage::GetLhsInfo)
            .def_property_readonly("same_table", &CompactMDEStorage::DefinedOnSameTable)
            .def_property_readonly("left_table", &CompactMDEStorage::GetLeftTableName)
            .def_property_readonly("right_table", &CompactMDEStorage::GetRightTableName)
            .def_property_readonly("search_space", &CompactMDEStorage::GetSearchSpaceSpecification)
            .def_property_readonly("mde_specifications", &CompactMDEStorage::GetMdeSpecifications);

    BindPrimitiveNoBase<HyMDE>(mde_module, "HyMDE").def("get_mdes", &HyMDE::GetMdes);
}
}  // namespace python_bindings
