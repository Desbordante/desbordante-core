#pragma once

#include <memory>
#include <string_view>
#include <typeindex>

#include "boost/any.hpp"

namespace algos::config {

class IOption {
public:
    virtual void Set(boost::any value) = 0;
    virtual void Unset() = 0;
    [[nodiscard]] virtual bool IsSet() const = 0;
    [[nodiscard]] virtual std::string_view GetName() const = 0;
    [[nodiscard]] virtual std::type_index GetTypeIndex() const = 0;
    virtual ~IOption() = default;
};

}  // namespace algos::config
