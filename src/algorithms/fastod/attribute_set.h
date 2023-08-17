#pragma once

#include <set>
#include <vector>
#include <string>

namespace algos::fastod {

class AttributeSet {
private:
    std::set<std::size_t> set_;
    unsigned long long value_;

public:
    AttributeSet() noexcept;
    explicit AttributeSet(std::size_t attribute) noexcept;
    explicit AttributeSet(const std::vector<std::size_t>& attributes) noexcept;
    explicit AttributeSet(const std::set<std::size_t>& set) noexcept;
    AttributeSet(AttributeSet& other) noexcept = default;
    AttributeSet(const AttributeSet& other) noexcept = default;

    bool ContainsAttribute(std::size_t attribute) const noexcept;
    AttributeSet AddAttribute(std::size_t attribute) const noexcept;
    AttributeSet DeleteAttribute(std::size_t attribute) const noexcept;

    AttributeSet Intersect(const AttributeSet& other) const noexcept;
    AttributeSet Union(const AttributeSet& other) const noexcept;
    AttributeSet Difference(const AttributeSet& other) const noexcept;

    bool IsEmpty() const noexcept;
    std::string ToString() const noexcept;

    std::size_t GetAttributeCount() const noexcept;
    unsigned long long GetValue() const noexcept;

    std::set<std::size_t>::iterator begin() const noexcept;
    std::set<std::size_t>::iterator end() const noexcept;

    friend bool operator==(AttributeSet const& x, AttributeSet const& y);
    friend bool operator<(const AttributeSet& x, const AttributeSet& y);

    AttributeSet& operator=(const AttributeSet& other);
};

} // namespace algos::fastod 
