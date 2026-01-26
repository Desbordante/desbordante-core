#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/pac/model/comparable_tuple_type.h"
#include "algorithms/pac/model/idomain.h"
#include "algorithms/pac/model/tuple.h"

namespace pac::model {
using StringValues = std::vector<std::string>;
using StringCompare = std::function<bool(StringValues const&, StringValues const&)>;
using StringDistFromDomain = std::function<double(StringValues const&)>;

/// @brief Domain that uses string value representations instead of typed values.
/// Useful when original type information is not needed (e. g. values to some specific type)
/// or cannot be obtained (e. g. Python bindings)
class UntypedDomain final : public IDomain {
private:
    StringCompare string_compare_;
    StringDistFromDomain string_dist_;
    std::string name_;

    StringValues ValuesToStrings(Tuple const& values) const {
        StringValues result(values.size());
        for (std::size_t i = 0; i < values.size(); ++i) {
            result[i] = tuple_type_->ByteToString(i, values[i]);
        }
        return result;
    }

public:
    /// @param string_compare -- function that returns true if first argument is less than second.
    /// @param string_dist_from_domain -- function that returns from this domain to its argument;
    /// second argument is dist_from_null_is_infinity.
    /// @param name -- displayed name of this domain; should be short, but informative.
    UntypedDomain(StringCompare&& string_compare, StringDistFromDomain&& string_dist_from_domain,
                  std::string&& name = "Untyped Domain")
        : string_compare_(std::move(string_compare)),
          string_dist_(std::move(string_dist_from_domain)),
          name_(std::move(name)) {}

    virtual double DistFromDomain(Tuple const& value) const override {
        return string_dist_(ValuesToStrings(value));
    }

    virtual std::string ToString() const override {
        return name_;
    }

    virtual void SetTypes(std::vector<::model::Type const*>&& types) override {
        tuple_type_ = std::make_shared<ComparableTupleType>(
                std::move(types), [this](Tuple const& a, Tuple const& b) {
                    return string_compare_(ValuesToStrings(a), ValuesToStrings(b));
                });
    }
};
}  // namespace pac::model
