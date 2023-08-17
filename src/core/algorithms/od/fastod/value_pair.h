#pragma once

#include <string>
#include "schema_value.h"

namespace algos::fastod {

// CHANGE: rename from DataAndIndex
// REASON: column types
class ValuePair {
private:
    std::pair<SchemaValue, SchemaValue> pair_;
    
public:
    ValuePair(SchemaValue const& data, SchemaValue const& index) noexcept;

    SchemaValue GetFirst() const noexcept;
    SchemaValue GetSecond() const noexcept;

    std::string ToString() const noexcept;
    friend bool operator<(const ValuePair& x, const ValuePair& y);
};

} //namespace algos::fastod
