#include "mde/bind_mde.h"

#include <cstddef>
#include <cstdint>
#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/typing.h>

#include "algorithms/mde/decision_boundaries/bool.h"
#include "algorithms/mde/decision_boundaries/decision_boundary.h"
#include "algorithms/mde/decision_boundaries/float.h"
#include "algorithms/mde/decision_boundaries/signed_integer.h"
#include "algorithms/mde/decision_boundaries/unsigned_integer.h"
#include "algorithms/mde/hymde/compact_mde_storage.h"
#include "algorithms/mde/hymde/hymde.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/equality.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/levenshtein_distance.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/levenshtein_similarity.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/monge_elkan_swg_word.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/number_difference.h"
#include "algorithms/mde/hymde/record_match_indexes/calculators/number_distance.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/custom.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/float_distance_ge.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/float_distance_le.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/float_ge.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/float_le.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/non_negative_float.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/predicate.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/predicate_inverse.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/signed_integer.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/signed_integer_ge.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/signed_integer_le.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/similarity.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/similarity_ge.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/similarity_le.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/total_order.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/unsigned_integer.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/unsigned_integer_ge.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/unsigned_integer_le.h"
#include "algorithms/mde/hymde/record_match_indexes/orders/valid_float.h"
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

template <typename Cls, typename... Args>
auto BindSharedPtr(Args&&... args) {
    return py::class_<Cls, std::shared_ptr<Cls>>(std::forward<Args>(args)...);
}

template <typename Cls, typename Base, typename... Args>
auto BindChildSharedPtr(Args&&... args) {
    return py::class_<Cls, Base, std::shared_ptr<Cls>>(std::forward<Args>(args)...);
}

template <typename BoundType>
auto BindDecisionBoundary(auto&& decision_boundaries_module, auto&& name) {
    using namespace model::mde::decision_boundaries;

    auto cls = BindChildSharedPtr<BoundType, DecisionBoundary>(decision_boundaries_module, name)
                       .def_property_readonly("value", &BoundType::GetValue);
    return cls;
}

template <typename PartFuncType>
auto BindPartFunc(auto&& module, auto&& name) {
    using namespace algos::hymde::record_match_indexes::partitioning_functions;

    using CreatorType = PartFuncType::Creator;

    auto cls =
            BindChildSharedPtr<CreatorType,
                               typename PartitioningFunction<typename PartFuncType::Type>::Creator>(
                    module, name);
    return cls;
}

template <typename OrderType, typename Base>
auto BindOrder(auto&& module, auto&& name) {
    auto cls = BindChildSharedPtr<OrderType, Base>(module, name).def(py::init<>());
    return cls;
}

template <typename SelectorType>
auto BindSizeLhsSelector(auto&& module, auto&& name) {
    using namespace algos::hymde::record_match_indexes::rcv_id_selectors;

    auto cls = BindChildSharedPtr<SelectorType, SizeBasedSelector>(module, name);
    if constexpr (std::is_constructible_v<SelectorType>) {
        cls.def(py::init<>());
    }
    return cls;
}

template <typename CalculatorType>
auto BindCalculatorCreator(auto&& calculator_specifiers, auto&& name) {
    using namespace algos::hymde::record_match_indexes::rcv_id_selectors;
    using namespace algos::hymde::record_match_indexes::calculators;

    using CreatorType = CalculatorType::Creator;
    auto cls =
            BindChildSharedPtr<CreatorType, Calculator::Creator>(calculator_specifiers, name)
                    .def(py::init<typename CreatorType::PartitioningFunctionCreatorsOption,
                                  typename CreatorType::OrderPtr, typename CreatorType::SelectorPtr,
                                  typename CreatorType::ComparisonResult>())
                    .def(py::init<typename CreatorType::PartitioningFunctionCreatorsOption,
                                  typename CreatorType::OrderPtr,
                                  std::shared_ptr<SizeBasedSelector const>,
                                  typename CreatorType::ComparisonResult>());
    return cls;
}

template <typename CalculatorType>
auto BindPredicateCalculatorCreator(auto&& calculator_specifiers, auto&& name) {
    using namespace algos::hymde::record_match_indexes::rcv_id_selectors;
    using namespace algos::hymde::record_match_indexes::calculators;

    using CreatorType = CalculatorType::Creator;
    auto cls = BindChildSharedPtr<CreatorType, Calculator::Creator>(calculator_specifiers, name)
                       .def(py::init<typename CreatorType::PartitioningFunctionCreatorsOption,
                                     typename CreatorType::OrderPtr,
                                     typename CreatorType::SelectorPtr>());
    return cls;
}

void BindDecisionBoundaries(py::module_& mde_module) {
    using namespace model::mde::decision_boundaries;

    auto decision_boundaries_module = mde_module.def_submodule("decision_boundaries");
    BindSharedPtr<DecisionBoundary>(decision_boundaries_module, "DecisionBoundary")
            .def("__str__", &DecisionBoundary::ToString);
    BindDecisionBoundary<Float>(decision_boundaries_module, "Float");
    BindDecisionBoundary<Bool>(decision_boundaries_module, "Bool");
    BindDecisionBoundary<SignedInteger>(decision_boundaries_module, "SignedInteger");
    BindDecisionBoundary<UnsignedInteger>(decision_boundaries_module, "UnsignedInteger");
}

void BindMdeParts(py::module_& mde_module) {
    using namespace model::mde;

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
}

void BindStringPartFuncs(py::module_& partitioning_functions) {
    using namespace algos::hymde::record_match_indexes::partitioning_functions;

    auto string_pf = partitioning_functions.def_submodule("string");
    BindSharedPtr<PartitioningFunction<std::string>::Creator>(string_pf, "PartitioningFunction");
    BindPartFunc<AttributeValue<std::string>>(string_pf, "AttributeValue")
            .def(py::init<AttributeValue<std::string>::Creator::ColumnIdentifier>());
}

void BindObjectPartFuncs(py::module_& partitioning_functions) {
    using namespace algos::hymde::record_match_indexes::partitioning_functions;

    auto object_pf = partitioning_functions.def_submodule("object_");
    py::class_<PartitioningFunction<py::object>>(object_pf, "PartitioningFunction");
    // TODO: py::object
}

void BindIntegerPartFuncs(py::module_& partitioning_functions) {
    using namespace algos::hymde::record_match_indexes::partitioning_functions;

    auto integer_pf = partitioning_functions.def_submodule("integer");
    BindSharedPtr<PartitioningFunction<std::intmax_t>::Creator>(integer_pf, "PartitioningFunction");
    BindPartFunc<AttributeValue<std::intmax_t>>(integer_pf, "AttributeValue")
            .def(py::init<AttributeValue<std::intmax_t>::Creator::ColumnIdentifier>());
}

void BindFloatPartFuncs(py::module_& partitioning_functions) {
    using namespace algos::hymde::record_match_indexes::partitioning_functions;

    auto float_pf = partitioning_functions.def_submodule("float_");
    BindSharedPtr<PartitioningFunction<double>::Creator>(float_pf, "PartitioningFunction");
    BindPartFunc<AttributeValue<double>>(float_pf, "AttributeValue")
            .def(py::init<AttributeValue<double>::Creator::ColumnIdentifier>());
}

void BindPartitioningFunctions(py::module_& mde_module) {
    auto partitioning_functions = mde_module.def_submodule("partitioning_functions");

    BindStringPartFuncs(partitioning_functions);
    BindObjectPartFuncs(partitioning_functions);
    BindIntegerPartFuncs(partitioning_functions);
    BindFloatPartFuncs(partitioning_functions);
}

template <typename BaseOrder>
void BindCustomOrder(py::module_& order_module) {
    using namespace py::literals;
    using ElementType = BaseOrder::Type;

    class ObjectComparer {
        py::object obj_;

    public:
        ObjectComparer(py::object obj) : obj_(std::move(obj)) {}

        bool operator()(ElementType const& el1, ElementType const& el2) const {
            return obj_(el1, el2).template cast<bool>();
        }
    };

    using namespace algos::hymde::record_match_indexes::orders;
    auto cls = BindChildSharedPtr<Custom<BaseOrder, ObjectComparer>, BaseOrder>(order_module,
                                                                                "CustomOrder")
                       .def(py::init<py::typing::Callable<bool(ElementType, ElementType)>,
                                     ElementType, ElementType, std::string>(),
                            "comparer"_a, "least_element"_a, "greatest_element"_a,
                            "display_name"_a = "(custom order)");
}

void BindSimOrders(py::module_& orders) {
    using namespace algos::hymde::record_match_indexes::orders;

    auto similarity_ord = orders.def_submodule("similarity");
    BindSharedPtr<Similarity>(similarity_ord, "BoundedTotalOrder");
    BindOrder<SimilarityLe, Similarity>(similarity_ord, "SimilarityLe");
    BindOrder<SimilarityGe, Similarity>(similarity_ord, "SimilarityGe");
    BindCustomOrder<Similarity>(similarity_ord);
}

void BindObjectOrders(py::module_& orders) {
    using namespace algos::hymde::record_match_indexes::orders;

    auto object_ord = orders.def_submodule("object_");
    BindSharedPtr<TotalOrder<py::object>>(object_ord, "BoundedTotalOrder");
    BindCustomOrder<TotalOrder<py::object>>(object_ord);
    // TODO: order that calls __le__
}

void BindBoolOrders(py::module_& orders) {
    using namespace algos::hymde::record_match_indexes::orders;

    auto predicate_ord = orders.def_submodule("predicate");
    BindSharedPtr<Bool>(predicate_ord, "BoundedTotalOrder");
    BindOrder<Predicate, Bool>(predicate_ord, "Predicate");
    BindOrder<PredicateInverse, Bool>(predicate_ord, "PredicateInverse");
}

void BindFloatOrders(py::module_& orders) {
    using namespace algos::hymde::record_match_indexes::orders;

    auto float_ord = orders.def_submodule("float_");
    BindSharedPtr<ValidFloat>(float_ord, "BoundedTotalOrder");
    BindOrder<FloatLe, ValidFloat>(float_ord, "FloatLe");
    BindOrder<FloatGe, ValidFloat>(float_ord, "FloatGe");
    BindCustomOrder<ValidFloat>(float_ord);
}

void BindFloatDistanceOrders(py::module_& orders) {
    using namespace algos::hymde::record_match_indexes::orders;

    auto float_dist_ord = orders.def_submodule("float_distance");
    BindSharedPtr<NonNegativeFloat>(float_dist_ord, "BoundedTotalOrder");
    BindOrder<FloatDistanceLe, NonNegativeFloat>(float_dist_ord, "FloatDistanceLe");
    BindOrder<FloatDistanceGe, NonNegativeFloat>(float_dist_ord, "FloatDistanceGe");
    BindCustomOrder<NonNegativeFloat>(float_dist_ord);
}

void BindSignedIntegerOrders(py::module_& orders) {
    using namespace algos::hymde::record_match_indexes::orders;

    auto signed_ord = orders.def_submodule("signed_integer");
    BindSharedPtr<SignedInteger>(signed_ord, "BoundedTotalOrder");
    BindOrder<SignedIntegerLe, SignedInteger>(signed_ord, "SignedIntegerLe");
    BindOrder<SignedIntegerGe, SignedInteger>(signed_ord, "SignedIntegerGe");
    BindCustomOrder<SignedInteger>(signed_ord);
}

void BindUnsignedIntegerOrders(py::module_& orders) {
    using namespace algos::hymde::record_match_indexes::orders;

    auto unsigned_ord = orders.def_submodule("unsigned_integer");
    BindSharedPtr<UnsignedInteger>(unsigned_ord, "BoundedTotalOrder");
    BindOrder<UnsignedIntegerLe, UnsignedInteger>(unsigned_ord, "UnsignedIntegerLe");
    BindOrder<UnsignedIntegerGe, UnsignedInteger>(unsigned_ord, "UnsignedIntegerGe");
    BindCustomOrder<UnsignedInteger>(unsigned_ord);
}

void BindOrders(py::module_& mde_module) {
    auto orders = mde_module.def_submodule("orders");

    BindSimOrders(orders);
    BindObjectOrders(orders);
    BindBoolOrders(orders);
    BindFloatOrders(orders);
    BindFloatDistanceOrders(orders);
    BindSignedIntegerOrders(orders);
    BindUnsignedIntegerOrders(orders);
}

void BindSizeSelectors(py::module_& lhs_selectors) {
    using namespace py::literals;
    using namespace algos::hymde::record_match_indexes::rcv_id_selectors;

    auto size_selectors = lhs_selectors.def_submodule("size");
    BindSharedPtr<SizeBasedSelector>(size_selectors, "Selector");
    BindSizeLhsSelector<Disable>(size_selectors, "Disable");
    BindSizeLhsSelector<MaxOnly>(size_selectors, "MaxOnly");
    BindSizeLhsSelector<IndexUniform>(size_selectors, "IndexUniform")
            .def(py::init<std::size_t>(), "indices"_a = 0);
}

void BindLhsSelectors(py::module_& mde_module) {
    using namespace algos::hymde::record_match_indexes::rcv_id_selectors;

    auto lhs_selectors = mde_module.def_submodule("lhs_selectors");

    BindSizeSelectors(lhs_selectors);

    auto double_selectors = lhs_selectors.def_submodule("float_");
    BindSharedPtr<Selector<double>>(double_selectors, "Selector");
    // TODO: custom selector.
}

void BindCalculatorSpecifiers(py::module_& mde_module) {
    using namespace algos::hymde::record_match_indexes::calculators;
    using namespace algos::hymde::record_match_indexes::orders;

    auto calculator_specifiers = mde_module.def_submodule("calculator_specifiers");

    BindSharedPtr<Calculator::Creator>(calculator_specifiers, "CalculatorSpecifier");
    BindCalculatorCreator<LevenshteinSimilarity>(calculator_specifiers, "LevenshteinSimilarity");
    BindCalculatorCreator<LevenshteinDistance>(calculator_specifiers, "LevenshteinDistance");
    BindCalculatorCreator<FloatDistance>(calculator_specifiers, "FloatDistance");
    BindCalculatorCreator<IntDistance>(calculator_specifiers, "IntDistance");
    BindCalculatorCreator<FloatDifference>(calculator_specifiers, "FloatDifference");
    BindCalculatorCreator<IntDifference>(calculator_specifiers, "IntDifference");
    BindCalculatorCreator<MongeElkan>(calculator_specifiers, "MongeElkan");
    BindPredicateCalculatorCreator<Equality<std::string>>(calculator_specifiers, "Equality");
    // TODO: custom
}

void BindMdeStorage(py::module_& mde_module) {
    using namespace algos::hymde;

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
}
}  // namespace

namespace python_bindings {
void BindMde(py::module_& main_module) {
    using HyMDE = algos::hymde::HyMDE;

    auto mde_module = main_module.def_submodule("mde");

    BindDecisionBoundaries(mde_module);
    BindMdeParts(mde_module);
    BindPartitioningFunctions(mde_module);
    BindOrders(mde_module);
    BindLhsSelectors(mde_module);
    BindCalculatorSpecifiers(mde_module);
    BindMdeStorage(mde_module);

    BindPrimitiveNoBase<HyMDE>(mde_module, "HyMDE").def("get_mdes", &HyMDE::GetMdes);
}
}  // namespace python_bindings
