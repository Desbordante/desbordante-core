#pragma once

#include <cstddef>
#include <memory>

namespace algos::cfdfinder {
enum class EntryType { kConstant = 0, kNegativeConstant, kVariable, kRange };

class Entry {
public:
    virtual ~Entry() = default;

    virtual bool Matches(size_t value) const = 0;
    virtual EntryType GetType() const = 0;
    virtual size_t Hash() const = 0;
    virtual bool operator==(Entry const& other) const = 0;
};

}  // namespace algos::cfdfinder