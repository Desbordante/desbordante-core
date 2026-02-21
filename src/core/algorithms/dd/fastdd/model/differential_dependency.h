#pragma once

#include <sstream>
#include <utility>
#include <vector>

#include "core/algorithms/dd/fastdd/model/differential_function.h"

namespace algos::dd {

class DifferentialDependency {
private:
    std::vector<DifferentialFunction> lhs_;
    DifferentialFunction rhs_;

public:
    DifferentialDependency(std::vector<DifferentialFunction> lhs, DifferentialFunction rhs)
        : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    std::vector<DifferentialFunction> const& GetLhs() const {
        return lhs_;
    }

    DifferentialFunction const& GetRhs() const {
        return rhs_;
    }

    std::string ToString() const {
        std::stringstream ss;
        ss << "{ ";
        for (auto const& df : lhs_) {
            ss << df.ToString() << ", ";
        }
        ss << "} -> ";
        ss << rhs_.ToString();

        return ss.str();
    }
};

}  // namespace algos::dd
