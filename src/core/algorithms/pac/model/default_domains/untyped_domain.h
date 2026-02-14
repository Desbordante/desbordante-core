#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/idomain.h"
#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/model/tuple_type.h"

namespace pac::model {
using StringValues = std::vector<std::string>;
using StringDistFromDomain = std::function<double(StringValues const&)>;

/// @brief Domain that uses string value representations instead of typed values.
/// Useful when original type information is not needed (e. g. values to some specific type)
/// or cannot be obtained (e. g. Python bindings)
class UntypedDomain final : public IDomain {
private:
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
    /// @param string_dist_from_domain -- function that calculates the distance from this domain to
    /// its argument
    /// @param name -- displayed name of this domain; should be short, but informative.
    UntypedDomain(StringDistFromDomain&& string_dist_from_domain,
                  std::string&& name = "Untyped Domain")
        : string_dist_(std::move(string_dist_from_domain)), name_(std::move(name)) {}

    virtual double DistFromDomain(Tuple const& value) const override {
        return string_dist_(ValuesToStrings(value));
    }

    virtual std::string ToString() const override {
        return name_;
    }

    virtual void SetTypes(std::vector<::model::Type const*>&& types) override {
        tuple_type_ = std::make_shared<TupleType>(std::move(types));
    }
};
}  // namespace pac::model
