#pragma once

#include "item_abstraction_pair.h"
#include <utility>
#include <vector>
#include <string>
#include <boost/dynamic_bitset.hpp>

namespace algos::cmspade{
class Pattern {
private:
    std::vector<ItemAbstractionPair> elements_;
    boost::dynamic_bitset<> appearing_;

public:
    Pattern() = default;

    Pattern(std::vector<ItemAbstractionPair> elements) 
        : elements_(std::move(elements)) {}

    Pattern(ItemAbstractionPair pair) {
        elements_.emplace_back(std::move(pair));
    }

    Pattern(const Pattern& other) = default;
    Pattern& operator=(const Pattern& other) = default;

    Pattern(Pattern&& other) = default;
    Pattern& operator=(Pattern&& other) = default;

    ~Pattern() = default;

    void Add(ItemAbstractionPair pair);
    std::size_t GetSupport() const { return appearing_.count(); }

    const boost::dynamic_bitset<>& GetAppearing() const { return appearing_;}
    void SetAppearing(boost::dynamic_bitset<> appearing) { appearing_ = std::move(appearing); }

    const std::vector<ItemAbstractionPair>& GetElements() const { return elements_; }
    const ItemAbstractionPair& GetElement(size_t index) const { return elements_[index]; }

    size_t Size() const { return elements_.size(); }

    const ItemAbstractionPair& GetLastElement() const;
    const ItemAbstractionPair& GetPenultimateElement() const;

    bool Equals(const Pattern& other) const;
    int CompareTo(const Pattern& other) const;

    bool operator==(const Pattern& other) const { return Equals(other); }
    bool operator<(const Pattern& other) const { return CompareTo(other) < 0; }

    Pattern Clone() const;
    bool IsPrefix(const Pattern& other) const;

    std::string ToString() const;
};
} // namespace algos::cmspade