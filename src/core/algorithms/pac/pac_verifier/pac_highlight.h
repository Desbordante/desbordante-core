#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/pac/model/tuple.h"
#include "type.h"

namespace algos::pac_verifier {
/// @brief Values that violate PAC with given epsilon
class PACHighlight {
public:
    virtual ~PACHighlight() = default;

    /// @brief Get row numbers of highlighted values
    virtual std::vector<std::size_t> GetRowNums() const = 0;

    /// @brief Get @c Types of highlighted columns
    virtual std::vector<model::Type const*> const& GetTypes() const = 0;

    /// @brief Get raw data of highlighted values.
    /// Can be converted to values using types (see @c GetTypes)
    virtual std::vector<pac::model::Tuple> GetByteData() const = 0;

    /// @brief Get highlighted values as strings
    virtual std::vector<std::string> GetStringData() const = 0;
};
}  // namespace algos::pac_verifier
