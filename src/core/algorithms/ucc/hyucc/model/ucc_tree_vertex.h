#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos::hyucc {

class UCCTreeVertex;

// Pair of a pointer to UCCTree node and corresponding UCC.
using LhsPair = std::pair<UCCTreeVertex*, boost::dynamic_bitset<>>;

class UCCTreeVertex {
private:
    std::vector<std::unique_ptr<UCCTreeVertex>> children_;
    size_t num_attributes_;
    bool is_ucc_;

    friend class UCCTree;

    [[nodiscard]] UCCTreeVertex* GetChild(size_t pos) const noexcept {
        assert(pos < children_.size());
        return children_[pos].get();
    }

    [[nodiscard]] bool IsObsolete() const noexcept {
        return !HasChildren() && !IsUCC();
    }

    void InitChildren(bool is_ucc = false);
    bool AddChild(size_t pos, bool is_ucc = false);
    void GetUCCAndGeneralizationsRecursiveImpl(boost::dynamic_bitset<> const& ucc, size_t cur_bit,
                                               boost::dynamic_bitset<> cur_ucc,
                                               std::vector<boost::dynamic_bitset<>>& res);
    [[nodiscard]] std::vector<boost::dynamic_bitset<>> GetUCCAndGeneralizationsRecursive(
            boost::dynamic_bitset<> const& ucc);
    void RemoveRecursive(boost::dynamic_bitset<> const& ucc, size_t cur_bit);
    [[nodiscard]] bool FindUCCOrGeneralizationRecursive(boost::dynamic_bitset<> const& ucc,
                                                        size_t cur_bit) const;
    void GetLevelRecursiveImpl(unsigned target_level, unsigned cur_level,
                               boost::dynamic_bitset<> ucc, std::vector<LhsPair>& result);
    void FillUCCsRecursive(std::vector<boost::dynamic_bitset<>>& uccs, boost::dynamic_bitset<> ucc);

    explicit UCCTreeVertex(size_t num_attributes, bool is_ucc)
        : num_attributes_(num_attributes), is_ucc_(is_ucc) {}

    static std::unique_ptr<UCCTreeVertex> Create(size_t num_attributes, bool is_ucc) {
        return std::unique_ptr<UCCTreeVertex>(new UCCTreeVertex(num_attributes, is_ucc));
    }

public:
    [[nodiscard]] size_t GetNumAttributes() const noexcept {
        return num_attributes_;
    }

    [[nodiscard]] bool HasChildren() const noexcept {
        return std::any_of(children_.begin(), children_.end(),
                           [](auto const& child) { return child != nullptr; });
    }

    [[nodiscard]] bool IsUCC() const noexcept {
        return is_ucc_;
    }

    void SetIsUCC(bool value) noexcept {
        is_ucc_ = value;
    }

    [[nodiscard]] bool ContainsChildAt(size_t pos) const noexcept {
        assert(pos < children_.size());
        return children_[pos] != nullptr;
    }

    [[nodiscard]] UCCTreeVertex* GetChildIfExists(size_t pos) const {
        if (children_.empty()) {
            return nullptr;
        }

        assert(pos < children_.size());
        return children_[pos].get();
    }

    [[nodiscard]] std::vector<LhsPair> GetLevelRecursive(unsigned target_level);
};

}  // namespace algos::hyucc
