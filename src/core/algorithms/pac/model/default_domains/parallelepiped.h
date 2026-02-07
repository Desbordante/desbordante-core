#pragma once

#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/default_domains/metric_based_domain.h"
#include "core/algorithms/pac/model/tuple.h"
#include "core/config/exceptions.h"

namespace pac::model {
/// @brief Closed n-ary parallelepiped, defined by two corners. D = [@c first, @c last] =
/// [@c first[1], @c last[1]] x ... x [@c first[n], @c last[n]]
class Parallelepiped final : public MetricBasedDomain {
private:
    Tuple first_;
    std::vector<std::string> first_str_;
    Tuple last_;
    std::vector<std::string> last_str_;
    std::vector<::model::Type::Destructor> destructors_;

protected:
    /// @brief Compare with Chebyshev distance
    virtual double DistFromDomainInternal(Tuple const& value) const override;
    virtual void ConvertValues() override;

public:
    Parallelepiped(std::vector<std::string>&& first_str, std::vector<std::string>&& last_str,
                   std::vector<double>&& leveling_coefficients = {})
        : MetricBasedDomain(std::move(leveling_coefficients)),
          first_str_(std::move(first_str)),
          last_str_(std::move(last_str)) {
        if (first_str_.size() != last_str_.size()) {
            throw config::ConfigurationError(
                    "Lower and upper bounds of Parallelepiped must contain the same number of "
                    "values");
        }
    }

    // 1D parallelepiped is an interval on a single column, so this syntactic sugar will be useful
    Parallelepiped(std::string&& single_first_str, std::string&& single_last_str)
        : first_str_{single_first_str}, last_str_{single_last_str} {}

    virtual ~Parallelepiped();
    virtual std::string ToString() const override;
};
}  // namespace pac::model
