#pragma once

#include <bit>
#include <memory>
#include <utility>

#include "algorithms/ind/faida/hashing/hashing.h"
#include "simple_cc.h"

namespace algos::faida {

class SimpleIND {
private:
    std::shared_ptr<SimpleCC> const left_;
    std::shared_ptr<SimpleCC> const right_;

public:
    SimpleIND(std::shared_ptr<SimpleCC> left, std::shared_ptr<SimpleCC> right)
        : left_(std::move(left)), right_(std::move(right)) {}

    bool operator==(SimpleIND const& other) const {
        return *(this->left_) == *(other.left_) && *(this->right_) == *(other.right_);
    }

    bool operator!=(SimpleIND const& other) const {
        return !(*this == other);
    }

    bool operator<(SimpleIND const& other) const;

    bool StartsWith(SimpleIND const& other) const {
        return this->left_->StartsWith(*other.left_) && this->right_->StartsWith(*other.right_);
    }

    std::shared_ptr<SimpleCC> const& left() const {
        return left_;
    }

    std::shared_ptr<SimpleCC> const& right() const {
        return right_;
    }

    size_t GetArity() const {
        return left_->GetColumnIndices().size();
    }
};

}  // namespace algos::faida

template <>
struct std::hash<algos::faida::SimpleIND> {
    size_t operator()(algos::faida::SimpleIND const& ind) const {
        size_t seed = 0;
        seed ^= reinterpret_cast<size_t>(ind.left().get());
        seed = std::rotl(seed, 11) ^ reinterpret_cast<size_t>(ind.right().get());
        return seed;
    }
};

template <>
struct std::hash<algos::faida::SimpleIND const*> {
    size_t operator()(algos::faida::SimpleIND const* ind) const {
        size_t seed = 0;
        seed ^= std::hash<algos::faida::SimpleCC>{}(*(ind->left()));
        seed = std::rotl(seed, 11) ^ std::hash<algos::faida::SimpleCC>{}(*(ind->right()));
        return seed;
    }
};

template <>
struct std::equal_to<algos::faida::SimpleIND const*> {
    bool operator()(algos::faida::SimpleIND const* a, algos::faida::SimpleIND const* b) const {
        return *a == *b;
    }
};
