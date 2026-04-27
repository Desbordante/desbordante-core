#include "python_bindings/tks/bind_tks.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/algorithms/tks/tks.h"
#include "python_bindings/py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace python_bindings {

void BindTKS(py::module_& main_module) {
    using namespace algos::tks;

    auto tks_module = main_module.def_submodule("tks");

    py::class_<Itemset>(tks_module, "Itemset")
            .def(py::init<>())
            .def(py::init<int>())
            .def("add_item", &Itemset::addItem, "Add an item to the itemset")
            .def("get_items", [](Itemset const& self) -> const std::vector<int>& {
                return self.getItems();
            }, py::return_value_policy::reference_internal,
                 "Get all items in the itemset")
            .def("size", &Itemset::size, "Number of items in the itemset")
            .def("contains", &Itemset::contains, "Check if itemset contains an item")
            .def("__len__", &Itemset::size)
            .def("__getitem__", [](Itemset const& self, size_t i) {
                if (i >= self.size()) throw py::index_error("index out of range");
                return self[i];
            })
            .def("__iter__",
                 [](Itemset const& self) {
                     auto const& items = self.getItems();
                     return py::make_iterator(items.begin(), items.end());
                 },
                 py::keep_alive<0, 1>())
            .def("__repr__", [](Itemset const& self) {
                std::string repr = "{";
                auto const& items = self.getItems();
                for (size_t i = 0; i < items.size(); ++i) {
                    if (i > 0) repr += " ";
                    repr += std::to_string(items[i]);
                }
                repr += "}";
                return repr;
            });

    py::class_<Prefix>(tks_module, "Prefix")
            .def(py::init<>())
            .def("add_itemset", &Prefix::addItemset, "Add an itemset to the sequence")
            .def("get_itemsets", [](Prefix const& self) -> const std::vector<Itemset>& {
                return self.getItemsets();
            }, py::return_value_policy::reference_internal,
                 "Get all itemsets in the prefix")
            .def("size", &Prefix::size, "Number of itemsets in the prefix")
            .def("get", (const Itemset& (Prefix::*)(size_t) const) &Prefix::get, py::return_value_policy::reference_internal,
                 "Get itemset by index", py::arg("index"))
            .def("get_item_occurrences_total_count", &Prefix::getItemOccurencesTotalCount,
                 "Total number of items in all itemsets")
            .def("contains_item", &Prefix::containsItem, "Check if prefix contains an item")
            .def("clone", &Prefix::cloneSequence, "Create a deep copy of the prefix")
            .def("to_string", &Prefix::toString, "Convert prefix to string representation")
            .def("__len__", &Prefix::size)
            .def("__getitem__", [](Prefix const& self, size_t i) {
                if (i >= self.size()) throw py::index_error("index out of range");
                return self.get(i);
            })
            .def("__iter__",
                 [](Prefix const& self) {
                     auto const& itemsets = self.getItemsets();
                     return py::make_iterator(itemsets.begin(), itemsets.end());
                 },
                 py::keep_alive<0, 1>())
            .def("__str__", &Prefix::toString)
            .def("__repr__", &Prefix::toString);

    py::class_<PatternTKS>(tks_module, "Pattern")
            .def_readonly("prefix", &PatternTKS::prefix, "The sequential pattern")
            .def_readonly("support", &PatternTKS::support, "The support (number of sequences)")
            .def("__str__", [](PatternTKS const& self) {
                std::string result = self.prefix.toString();
                result += " | support: " + std::to_string(self.support);
                return result;
            })
            .def("__repr__", [](PatternTKS const& self) {
                std::string result = self.prefix.toString();
                result += " | support: " + std::to_string(self.support);
                return result;
            })
            .def(py::pickle(
                    [](PatternTKS const& pattern) {
                        auto const& itemsets = pattern.prefix.getItemsets();
                        std::vector<std::vector<int>> itemsets_as_vectors;
                        for (auto const& itemset : itemsets) {
                            itemsets_as_vectors.push_back(std::vector<int>(
                                    itemset.getItems().begin(), itemset.getItems().end()));
                        }
                        return py::make_tuple(itemsets_as_vectors, pattern.support);
                    },
                    // __setstate__
                    [](py::tuple tup) {
                        if (tup.size() != 2) {
                            throw std::runtime_error("Invalid state for Pattern pickle!");
                        }
                        auto itemsets_vec = tup[0].cast<std::vector<std::vector<int>>>();
                        int support = tup[1].cast<int>();

                        Prefix prefix;
                        for (auto const& items : itemsets_vec) {
                            Itemset itemset;
                            for (int item : items) {
                                itemset.addItem(item);
                            }
                            prefix.addItemset(itemset);
                        }
                        return PatternTKS(std::move(prefix), support);
                    }));

    BindPrimitiveNoBase<algos::tks::TKS>(tks_module, "TKS")
            .def("get_patterns", &algos::tks::TKS::GetPatterns,
                 py::return_value_policy::reference_internal, "Get the discovered patterns")
            .def("get_pattern_count", &algos::tks::TKS::GetPatternCount,
                 "Get the number of discovered patterns")
            .def("_get_patterns_list", [](algos::tks::TKS& algo) {
                auto const& patterns_queue = algo.GetPatterns();
                std::vector<PatternTKS> patterns_list;
                auto temp_queue = patterns_queue;
                while (!temp_queue.empty()) {
                    patterns_list.push_back(temp_queue.top());
                    temp_queue.pop();
                }
                return patterns_list;
            },
                    "Get the patterns as a Python list (for easier iteration)");

    main_module.attr("tks_module") = tks_module;
}

}  // namespace python_bindings
