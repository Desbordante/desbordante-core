#pragma once

#include <string>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "item_abstraction_pair.h"

namespace algos::cmspade {
class Pattern {
private:
    std::vector<ItemAbstractionPair> elements_;
    boost::dynamic_bitset<> appearing_;

public:
    Pattern() = default;

    Pattern(std::vector<ItemAbstractionPair> elements) : elements_(std::move(elements)) {}

    Pattern(ItemAbstractionPair pair) {
        elements_.emplace_back(std::move(pair));
    }

    Pattern(Pattern const& other) = default;
    Pattern& operator=(Pattern const& other) = default;

    Pattern(Pattern&& other) = default;
    Pattern& operator=(Pattern&& other) = default;

    ~Pattern() = default;

    void Add(ItemAbstractionPair pair);

    std::size_t GetSupport() const {
        return appearing_.count();
    }

    boost::dynamic_bitset<> const& GetAppearing() const {
        return appearing_;
    }

    void SetAppearing(boost::dynamic_bitset<> appearing) {
        appearing_ = std::move(appearing);
    }

    std::vector<ItemAbstractionPair> const& GetElements() const {
        return elements_;
    }

    ItemAbstractionPair const& GetElement(size_t index) const {
        return elements_[index];
    }

    size_t Size() const {
        return elements_.size();
    }

    ItemAbstractionPair const& GetLastElement() const;
    ItemAbstractionPair const& GetPenultimateElement() const;

    bool Equals(Pattern const& other) const;
    int CompareTo(Pattern const& other) const;

    bool operator==(Pattern const& other) const {
        return Equals(other);
    }

    bool operator<(Pattern const& other) const {
        return CompareTo(other) < 0;
    }

    Pattern Clone() const;
    bool IsPrefix(Pattern const& other) const;

    std::string ToString() const;
};
}  // namespace algos::cmspade