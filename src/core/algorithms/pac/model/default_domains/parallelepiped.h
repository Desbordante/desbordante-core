#pragma once

#include <string>
#include <vector>

#include "algorithms/pac/model/default_domains/metric_based_domain.h"
#include "algorithms/pac/model/tuple.h"

namespace pac::model {
/// @brief Closed n-ary parallelepiped, defined by two corners. D = [@c first, @c last] =
/// [@c first[0], @c last[0]] x [@c first[1], @c last[1]] x ... x [@c first[n], @c last[n]]
class Parallelepiped final : public MetricBasedDomain {
private:
    Tuple first_;
    std::vector<std::string> first_str_;
    Tuple last_;
    std::vector<std::string> last_str_;
    std::vector<::model::Type::Destructor> destructors_;

    /// @brief Chebyshev distance, also known as \rho_\infty or chessboard distance
    /// d(X, Y) = max{|x[0] - y[0]|, |x[1] - y[1]|, ..., |x[n] - y[n]|}
    double ChebyshevDist(Tuple const& x, Tuple const& y) const;

    /// @c Compare using product order.
    /// X < Y iff x[0] < y[0] & x[1] < y[1] & ... & x[n] < y[n]
    bool ProductCompare(Tuple const& x, Tuple const& y) const;

protected:
    virtual double DistFromDomainInternal(Tuple const& value) const override;
    virtual void ConvertValues() override;

    /// @c Compare distances between value and bottom-left corner of parallelepiped:
    /// X < Y iff d(X, D) < d(Y, D)
    virtual bool CompareInternal(Tuple const& x, Tuple const& y) const override;

public:
    Parallelepiped(std::vector<std::string>&& first_str, std::vector<std::string>&& last_str,
                   std::vector<double>&& leveling_coefficients = {})
        : MetricBasedDomain(std::move(leveling_coefficients)),
          first_str_(std::move(first_str)),
          last_str_(std::move(last_str)) {}

    // 1D parallelepiped is an interval on a single column, so this syntactic sugar will be useful
    Parallelepiped(std::string&& single_first_str, std::string&& single_last_str)
        : first_str_{single_first_str}, last_str_{single_last_str} {}

    virtual ~Parallelepiped();
    virtual std::string ToString() const override;
};
}  // namespace pac::model
