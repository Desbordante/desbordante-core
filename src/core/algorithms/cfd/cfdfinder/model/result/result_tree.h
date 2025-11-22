#pragma once

#include <list>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/cfd/cfdfinder/util/lhs_utils.h"
#include "raw_cfd.h"

namespace algos::cfdfinder {
class ResultTree {
private:
    RawCFD root_;
    std::list<ResultTree> children_;

    void AppendLeaves(ResultTree* node, std::list<RawCFD>& leaves) {
        if (node->children_.empty()) {
            leaves.push_back(std::move(node->root_));
            return;
        }
        for (auto& child : node->children_) {
            AppendLeaves(&child, leaves);
        }
    }

    ResultTree* FindNode(boost::dynamic_bitset<> const& lhs, size_t rhs) {
        auto const& embedded_fd = root_.embedded_fd_;
        if (embedded_fd.rhs_ != rhs) {
            return nullptr;
        }
        if (embedded_fd.lhs_ == lhs) {
            return this;
        }
        for (auto& child : children_) {
            auto const& result = child.FindNode(lhs, rhs);
            if (result != nullptr) {
                return result;
            }
        }
        return nullptr;
    }

public:
    ResultTree(RawCFD root) : root_(std::move(root)) {}

    double GetSupportRoot() const {
        return root_.patterns_.GetSupport();
    }

    RawCFD const& GetRoot() const {
        return root_;
    }

    std::list<ResultTree> const& GetChildren() const {
        return children_;
    }

    ResultTree* GetInsertPosition(RawCFD const& result) {
        ResultTree* child;
        for (auto const& bitset : util::GenerateLhsSupersets(result.embedded_fd_.lhs_)) {
            child = FindNode(bitset, result.embedded_fd_.rhs_);
            if (child != nullptr) {
                return child;
            }
        }
        return nullptr;
    }

    void AddChild(RawCFD result) {
        children_.emplace_back(std::move(result));
    }

    std::list<RawCFD> GetLeaves() {
        std::list<RawCFD> leaves;
        AppendLeaves(this, leaves);
        return leaves;
    }
};
}  // namespace algos::cfdfinder
