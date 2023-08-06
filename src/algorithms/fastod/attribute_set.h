#pragma once

#include <set>
#include <vector>
#include <string>

namespace algos::fastod {

class AttributeSet {
private:
    std::set<int> set_;
    unsigned long long value_;

public:
    AttributeSet() noexcept;
    AttributeSet(int attribute) noexcept;
    AttributeSet(const std::vector<int>& attributes) noexcept;
    AttributeSet(const std::set<int>& set) noexcept;

    bool ContainsAttribute(int attribute) const noexcept;
    AttributeSet AddAttribute(int attribute) const noexcept;
    AttributeSet DeleteAttribute(int attribute) const noexcept;

    AttributeSet Intersect(const AttributeSet& other) const noexcept;
    AttributeSet Union(const AttributeSet& other) const noexcept;
    AttributeSet Difference(const AttributeSet& other) const noexcept;

    bool IsEmpty() const noexcept;
    std::string ToString() const noexcept;

    std::size_t GetAttributeCount() const noexcept;
    unsigned long long GetValue() const noexcept;

    std::set<int>::iterator begin() const noexcept;
    std::set<int>::iterator end() const noexcept;

    friend bool operator==(AttributeSet const& x, AttributeSet const& y);
    friend bool operator<(const AttributeSet& x, const AttributeSet& y);

    AttributeSet& operator=(const AttributeSet& other);
};

} // namespace algos::fastod 
