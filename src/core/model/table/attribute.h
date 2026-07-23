#pragma once

#include <string>

#include "core/model/index.h"

namespace model {
struct Attribute {
    std::string name;
    Index id;

    bool operator==(Attribute const& other) const noexcept {
        return other.name == name && other.id == id;
    }

    Attribute(std::string name, Index id) : name(std::move(name)), id(id) {}
};
}  // namespace model
