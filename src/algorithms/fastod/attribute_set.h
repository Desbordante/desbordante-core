# pragma once

#include <set>
#include <vector>
#include <string>

namespace algos::fastod {

class AttributeSet {
private:
    std::set<int> const set_;
    unsigned long long const value_;

public:
    AttributeSet() noexcept;
    AttributeSet(int attribute) noexcept;
    AttributeSet(std::vector<int> attributes) noexcept;
    AttributeSet(std::set<int> set) noexcept;

    bool ContainsAttribute(int attribute) const noexcept;
    AttributeSet AddAttribute(int attribute) const noexcept;
    AttributeSet DeleteAttribute(int attribute) const noexcept;

    AttributeSet Intersect(AttributeSet other) const noexcept;
    AttributeSet Union(AttributeSet other) const noexcept;
    AttributeSet Difference(AttributeSet other) const noexcept;

    bool IsEmpty() const noexcept;
    std::string ToString() const noexcept;

    std::size_t GetAttributeCount() const noexcept;
    unsigned long long GetValue() const noexcept;

    std::set<int>::iterator begin() const noexcept;
    std::set<int>::iterator end() const noexcept;

    friend bool operator==(AttributeSet const& x, AttributeSet const& y);
};

} // namespace algos::fastod 
