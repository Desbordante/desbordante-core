#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "algorithms/pac/model/comparable_tuple_type.h"
#include "algorithms/pac/model/default_domains/metric_based_domain.h"
#include "imetrizable_type.h"

namespace pac::model {
/// @brief n-ary parallelepiped, defined by value vectors @c first and @c last, i. e.
/// D = [first[0], last[0]] x [first[1], last[1]] x ... x [first[n], last[n]]
class Parallelepiped : public MetricBasedDomain {
private:
    Tuple first_;
    Tuple last_;
    std::vector<std::string> first_str_;
    std::vector<std::string> last_str_;

    /// @brief Chebyshev distance, also known as l_\infty metric, sup-metric or chessboard metric.
    /// d(X, Y) = max{|x[0] - y[0]|, |x[1] - y[1]|, ..., |x[n] - y[n]|}
    double ChebyshevDist(Tuple const& x, Tuple const& y) const {
        double max_dist = -1;
        for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
            max_dist = std::max(max_dist, metrizable_types_[i]->Dist(x[i], y[i]));
        }
        return max_dist;
    }

protected:
    /// @brief X < Y iff x1 < y1 & x2 < y2 & ... & xn < yn.
    /// Thus, (X, Y) = (x1, y1) x (x2, y2) x ... x (xn, yn), which is n-ary parallelepiped
    virtual bool Compare(Tuple const& x, Tuple const& y) const override {
        for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
            auto comp_res = metrizable_types_[i]->Compare(x[i], y[i]);
            if (comp_res != ::model::CompareResult::kLess) {
                return false;
            }
        }
        return true;
    };

    virtual double DistFromDomainInternal(Tuple const& value) const override {
        if (tuple_type_->Less(value, first_)) {
            return ChebyshevDist(value, first_);
        } else if (tuple_type_->Less(last_, value)) {
            return ChebyshevDist(value, last_);
        }
        return 0;
    }

    virtual void ConvertValues() override {
        first_ = AllocateValues(first_str_);
        last_ = AllocateValues(last_str_);
    }

public:
    /// @brief Use data as @c first and @c last and hold it in this Domain object
    Parallelepiped(std::vector<std::string>&& first_str, std::vector<std::string>&& last_str)
        : first_str_(std::move(first_str)), last_str_(std::move(last_str)) {}

    virtual ~Parallelepiped() {
        FreeValues(first_);
        FreeValues(last_);
    }

    virtual std::string ToString() const override {
        std::ostringstream oss;
        oss << '(' << tuple_type_->ValueToString(first_) << ", "
            << tuple_type_->ValueToString(last_) << ')';
        return oss.str();
    }
};
}  // namespace pac::model
