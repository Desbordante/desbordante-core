#include <numeric>
#include <vector>
#include <algorithm>
#include <sstream>

#include "attribute_set.h"

using namespace algos::fastod;

AttributeSet::AttributeSet() noexcept : set_(), value_(0) {}

AttributeSet::AttributeSet(size_t attribute) noexcept : set_({attribute}), value_(1 << attribute) {}

AttributeSet::AttributeSet(const std::vector<size_t>& attributes) noexcept :
    set_(attributes.cbegin(), attributes.cend()),
    value_(std::accumulate(attributes.cbegin(), attributes.cend(), 0, [](const unsigned long long& acc, const size_t& curr){ return acc + (1 << curr); })) {}

AttributeSet::AttributeSet(const std::set<size_t>& set) noexcept :
    set_(set),
    value_(std::accumulate(set.cbegin(), set.cend(), 0, [](const unsigned long long& acc, const size_t& curr){ return acc + (1 << curr); })) {}

bool AttributeSet::ContainsAttribute(size_t attribute) const noexcept {
    return set_.find(attribute) != set_.end();
}

AttributeSet AttributeSet::AddAttribute(size_t attribute) const noexcept {
    if (ContainsAttribute(attribute)) {
        return *this;
    }

    std::set<size_t> new_set = set_;
    new_set.insert(attribute);

    return AttributeSet(std::move(new_set));
}

AttributeSet AttributeSet::DeleteAttribute(size_t attribute) const noexcept {
    if (!ContainsAttribute(attribute)) {
        return *this;
    }

    std::set<size_t> new_set = set_;
    new_set.erase(attribute);

    return AttributeSet(std::move(new_set));
}

AttributeSet AttributeSet::Intersect(const AttributeSet& other) const noexcept {
    std::vector<size_t> result;
    std::set_intersection(set_.begin(), set_.end(), other.set_.begin(), other.set_.end(), std::back_inserter(result));

    return AttributeSet(std::move(result));
}

AttributeSet AttributeSet::Union(const AttributeSet& other) const noexcept {
    std::vector<size_t> result;
    std::set_union(set_.begin(), set_.end(), other.set_.begin(), other.set_.end(), std::back_inserter(result));

    return AttributeSet(std::move(result));
}

AttributeSet AttributeSet::Difference(const AttributeSet& other) const noexcept {
    std::vector<size_t> result;
    std::set_difference(set_.begin(), set_.end(), other.set_.begin(), other.set_.end(), std::back_inserter(result));

    return AttributeSet(std::move(result));
}

bool AttributeSet::IsEmpty() const noexcept {
    return set_.empty();
}

std::string AttributeSet::ToString() const noexcept {
   std::stringstream ss;

   ss << "{";

   bool first = true;

   for (size_t attribute: set_) {
        if (first) {
            first = false;
        } else {
            ss << ",";
        }

        ss << attribute + 1;
   }

   ss << "}";

   return ss.str();
}

std::size_t AttributeSet::GetAttributeCount() const noexcept {
    return set_.size();
}

unsigned long long AttributeSet::GetValue() const noexcept {
    return value_;
}

std::set<size_t>::iterator AttributeSet::begin() const noexcept {
    return set_.begin();
}

std::set<size_t>::iterator AttributeSet::end() const noexcept {
    return set_.end();
}

AttributeSet& AttributeSet::operator=(const AttributeSet& other) {
    if (this == &other) {
        return *this;
    }

    set_ = other.set_;
    value_ = other.value_;

    return *this;
}

namespace algos::fastod {

bool operator==(AttributeSet const& x, AttributeSet const& y) {
    return x.set_ == y.set_;
}

bool operator<(const AttributeSet& x, const AttributeSet& y) {
    return x.value_ < y.value_;
}

} // namespace algos::fastod
