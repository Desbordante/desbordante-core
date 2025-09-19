#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

#include <boost/format/detail/compat_workarounds.hpp>

#include "exceptions.h"
#include "imetrizable_type.h"
#include "pac/model/comparable_tuple_type.h"
#include "pac/model/idomain.h"
#include "type.h"

namespace pac::model {
/// @brief Utility class for IMetrizableType-based Domains
class MetricBasedDomain : public IDomain {
protected:
    std::vector<::model::IMetrizableType const*> metrizable_types_ = {};

    /// @brief Comparer bound to this domain type
    virtual bool Compare(Tuple const& x, Tuple const& y) const = 0;

    /// @brief Metric bound to this domain type
    virtual double DistFromDomainInternal(Tuple const& value) const = 0;

    /// @brief Convert all needed values after types are set
    virtual void ConvertValues() {}

    std::vector<std::byte const*> AllocateValues(std::vector<std::string> const& str_values) const {
        assert(str_values.size() >= metrizable_types_.size());

        std::vector<std::byte const*> values;
        for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
            auto const& type = *metrizable_types_[i];
            auto* ptr = type.Allocate();
            type.ValueFromStr(ptr, str_values[i]);
            values.push_back(ptr);
        }
        return values;
    }

    void FreeValues(std::vector<std::byte const*> const& values) const {
        assert(values.size() >= metrizable_types_.size());

        for (std::size_t i = 0; i < metrizable_types_.size(); ++i) {
            metrizable_types_[i]->Free(values[i]);
        }
    }

public:
    virtual void SetTypes(std::vector<::model::Type const*>&& types) override {
        using namespace std::placeholders;

        std::transform(types.begin(), types.end(), std::back_inserter(metrizable_types_),
                       [](::model::Type const* type) {
                           auto const* metrizable_type =
                                   dynamic_cast<::model::IMetrizableType const*>(type);
                           if (!metrizable_type) {
                               throw config::ConfigurationError(
                                       "To use metric-based domain, all affected columns must have "
                                       "metrizable type");
                           }
                           return metrizable_type;
                       });
        tuple_type_ = std::make_shared<ComparableTupleType>(
                std::move(types), std::bind(&MetricBasedDomain::Compare, this, _1, _2));
        ConvertValues();
    }

    virtual double DistFromDomain(Tuple const& value) const override {
		std::cout << "Dist from " << tuple_type_->ValueToString(value) << " to " << ToString() << '\n';
        if (metrizable_types_.empty()) {
            throw std::runtime_error("SetTypes must be called before using MetricBasedDomain");
        }
        assert(value.size() >= metrizable_types_.size());

        return DistFromDomainInternal(value);
    }
};
}  // namespace pac::model
