#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include <boost/dynamic_bitset.hpp>
#include <boost/functional/hash.hpp>

namespace algos::fd::fdhits::edges {
class DefaultEdge {
private:
    boost::dynamic_bitset<> bits_;

    explicit DefaultEdge(boost::dynamic_bitset<> bits) : bits_(std::move(bits)) {}

public:
    static constexpr auto kNpos = boost::dynamic_bitset<>::npos;

    explicit DefaultEdge(size_t size) : bits_(size) {}

    static DefaultEdge Empty(size_t size) {
        return DefaultEdge(size);
    }

    static DefaultEdge Filled(size_t size) {
        DefaultEdge edge(size);
        edge.bits_.set();
        return edge;
    }

    void Add(size_t node) {
        bits_.set(node);
    }

    void Remove(size_t node) {
        bits_.reset(node);
    }

    bool Contains(size_t node) const {
        return bits_.test(node);
    }

    size_t Count() const {
        return bits_.count();
    }

    size_t Size() const {
        return bits_.size();
    }

    bool IsEmpty() const {
        return bits_.none();
    }

    size_t Overlap(DefaultEdge const& other) const {
        return (bits_ & other.bits_).count();
    }

    bool Covers(DefaultEdge const& other) const {
        return bits_.intersects(other.bits_);
    }

    bool Implies(DefaultEdge const& other) const {
        return bits_.is_subset_of(other.bits_);
    }

    bool IsImpliedBy(std::vector<DefaultEdge> const& others) const {
        for (auto const& e : others) {
            if (e.Implies(*this)) return true;
        }
        return false;
    }

    void RemoveAll(DefaultEdge const& other) {
        bits_ -= other.bits_;
    }

    void AddAll(DefaultEdge const& other) {
        bits_ |= other.bits_;
    }

    DefaultEdge Intersect(DefaultEdge const& other) const {
        return DefaultEdge(bits_ & other.bits_);
    }

    void IntersectWith(DefaultEdge const& other) {
        bits_ &= other.bits_;
    }

    void SetTo(DefaultEdge const& other) {
        bits_ = other.bits_;
    }

    void Clear() {
        bits_.reset();
    }

    std::vector<size_t> GetNodes() const {
        std::vector<size_t> nodes;
        nodes.reserve(Count());
        for (size_t i = bits_.find_first(); i != kNpos; i = bits_.find_next(i)) {
            nodes.push_back(i);
        }
        return nodes;
    }

    class NodesIterator {
    private:
        boost::dynamic_bitset<> const* bits_;
        size_t current_;

    public:
        NodesIterator(boost::dynamic_bitset<> const* bits, size_t pos)
            : bits_(bits), current_(pos) {}

        size_t operator*() const {
            return current_;
        }

        NodesIterator& operator++() {
            current_ = bits_->find_next(current_);
            return *this;
        }

        bool operator!=(NodesIterator const& other) const {
            return current_ != other.current_;
        }
    };

    NodesIterator NodesBegin() const {
        return NodesIterator(&bits_, bits_.find_first());
    }

    NodesIterator NodesEnd() const {
        return NodesIterator(&bits_, kNpos);
    }

    std::string ToString() const {
        std::ostringstream oss;
        bool first = true;
        for (size_t i = bits_.find_first(); i != kNpos; i = bits_.find_next(i)) {
            if (!first) oss << ",";
            oss << i;
            first = false;
        }
        return oss.str();
    }

    bool operator==(DefaultEdge const& other) const {
        return bits_ == other.bits_;
    }

    bool operator!=(DefaultEdge const& other) const {
        return bits_ != other.bits_;
    }

    bool operator<(DefaultEdge const& other) const {
        if (bits_.size() != other.bits_.size()) {
            return bits_.size() < other.bits_.size();
        }
        for (size_t i = 0; i < bits_.size(); ++i) {
            if (bits_[i] != other.bits_[i]) {
                return bits_[i] < other.bits_[i];
            }
        }
        return false;
    }

    boost::dynamic_bitset<>& GetBits() {
        return bits_;
    }

    boost::dynamic_bitset<> const& GetBits() const {
        return bits_;
    }

    struct Hash {
        std::size_t operator()(DefaultEdge const& edge) const {
            std::size_t hash = 0;
            boost::hash_combine(hash, edge.bits_.count());
            for (size_t i = edge.bits_.find_first(); i != kNpos; i = edge.bits_.find_next(i)) {
                boost::hash_combine(hash, i);
            }
            return hash;
        }
    };
};

}  // namespace algos::fd::fdhits::edges
