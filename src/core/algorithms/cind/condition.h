#pragma once

#include <vector>

#include <boost/container_hash/hash.hpp>

namespace algos::cind {

struct Condition {
    std::vector<int> attrs;
    std::vector<int> attrs_values;
    double validity;
    double completeness;

    Condition(std::vector<int> const& _attrs, std::vector<int> const& _attrs_values,
              double _validity, double _completeness)
        : attrs(_attrs),
          attrs_values(_attrs_values),
          validity(_validity),
          completeness(_completeness) {}

    Condition() = default;
    ~Condition() = default;
};
}  // namespace algos::cind

template <>
struct std::hash<algos::cind::Condition> {
    size_t operator()(algos::cind::Condition const& cond) const noexcept {
        size_t result = 0;
        boost::hash_combine(result, boost::hash_value(cond.validity));
        boost::hash_combine(result, boost::hash_value(cond.completeness));
        boost::hash_combine(result, boost::hash_value(cond.completeness));
        return result;  // or use boost::hash_combine
    }
};