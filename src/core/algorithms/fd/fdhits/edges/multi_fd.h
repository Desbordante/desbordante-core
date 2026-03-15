#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <boost/functional/hash.hpp>

#include "core/algorithms/fd/fdhits/edges/default_edge.h"

namespace algos::fd::fdhits::edges {
class MultiFD {
private:
    DefaultEdge lhs_;
    DefaultEdge rhs_;

public:
    MultiFD(size_t num_columns) : lhs_(num_columns / 2), rhs_(num_columns / 2) {}

    MultiFD(DefaultEdge lhs, DefaultEdge rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    static MultiFD Empty(size_t num_columns) {
        return MultiFD(num_columns);
    }

    static MultiFD Filled(size_t num_columns) {
        return MultiFD(DefaultEdge::Filled(num_columns / 2), DefaultEdge::Filled(num_columns / 2));
    }

    void Add(size_t node) {
        if (node < lhs_.Size()) {
            lhs_.Add(node);
        } else {
            rhs_.Add(node - lhs_.Size());
        }
    }

    void Remove(size_t node) {
        if (node < lhs_.Size()) {
            lhs_.Remove(node);
        } else {
            rhs_.Remove(node - lhs_.Size());
        }
    }

    void Clear() {
        lhs_.Clear();
        rhs_.Clear();
    }

    bool Contains(size_t node) const {
        if (node < lhs_.Size()) {
            return lhs_.Contains(node);
        } else {
            return rhs_.Contains(node - lhs_.Size());
        }
    }

    size_t Count() const {
        return lhs_.Count() + rhs_.Count();
    }

    size_t Size() const {
        return lhs_.Size() + rhs_.Size();
    }

    size_t Overlap(MultiFD const& other) const {
        return lhs_.Overlap(other.lhs_) + rhs_.Overlap(other.rhs_);
    }

    bool Covers(MultiFD const& other) const {
        return lhs_.Covers(other.lhs_) || rhs_.Implies(other.rhs_);
    }

    bool Implies(MultiFD const& other) const {
        return lhs_.Implies(other.lhs_) && rhs_.Implies(other.rhs_);
    }

    bool IsImpliedBy(std::vector<MultiFD> const& others) const {
        return std::any_of(others.begin(), others.end(),
                           [this](MultiFD const& o) { return o.Implies(*this); });
    }

    void RemoveAll(MultiFD const& other) {
        lhs_.RemoveAll(other.lhs_);
        rhs_.RemoveAll(other.rhs_);
    }

    void AddAll(MultiFD const& other) {
        lhs_.AddAll(other.lhs_);
        rhs_.AddAll(other.rhs_);
    }

    MultiFD Intersect(MultiFD const& other) const {
        return MultiFD(lhs_.Intersect(other.lhs_), rhs_.Intersect(other.rhs_));
    }

    void RemoveAllInvalid(MultiFD const& other) {
        for (size_t rhs_node : other.rhs_.GetNodes()) {
            lhs_.Remove(rhs_node);
        }
        if (other.rhs_.Count() > 0) {
            rhs_.Clear();
        } else {
            for (size_t lhs_node : other.lhs_.GetNodes()) {
                rhs_.Remove(lhs_node);
            }
        }
    }

    void SetTo(MultiFD const& other) {
        lhs_.SetTo(other.lhs_);
        rhs_.SetTo(other.rhs_);
    }

    std::vector<size_t> GetNodes() const {
        std::vector<size_t> nodes;
        nodes.reserve(Count());
        for (size_t node : lhs_.GetNodes()) {
            nodes.push_back(node);
        }
        size_t offset = lhs_.Size();
        for (size_t node : rhs_.GetNodes()) {
            nodes.push_back(node + offset);
        }
        return nodes;
    }

    std::string ToString() const {
        return lhs_.ToString() + " -> " + rhs_.ToString();
    }

    DefaultEdge const& GetLhs() const {
        return lhs_;
    }

    DefaultEdge const& GetRhs() const {
        return rhs_;
    }

    std::vector<size_t> GetRhsNodes() const {
        return rhs_.GetNodes();
    }

    void RemoveRhs(size_t node) {
        rhs_.Remove(node);
    }

    void RemoveLhs(size_t node) {
        lhs_.Remove(node);
    }

    void AddLhs(size_t node) {
        lhs_.Add(node);
    }

    void SetRhs(DefaultEdge const& rhs) {
        rhs_.SetTo(rhs);
    }

    void IntersectRhs(DefaultEdge const& other) {
        rhs_.IntersectWith(other);
    }

    size_t RhsCount() const {
        return rhs_.Count();
    }

    bool operator==(MultiFD const& other) const {
        return lhs_ == other.lhs_ && rhs_ == other.rhs_;
    }

    bool operator!=(MultiFD const& other) const {
        return !(*this == other);
    }

    bool operator<(MultiFD const& other) const {
        if (lhs_ != other.lhs_) return lhs_ < other.lhs_;
        return rhs_ < other.rhs_;
    }

    struct Hash {
        std::size_t operator()(MultiFD const& fd) const {
            std::size_t hash = 0;
            boost::hash_combine(hash, DefaultEdge::Hash{}(fd.lhs_));
            boost::hash_combine(hash, DefaultEdge::Hash{}(fd.rhs_));
            return hash;
        }
    };
};

}  // namespace algos::fd::fdhits::edges
