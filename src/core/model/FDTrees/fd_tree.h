#pragma once

#include <memory>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/fd/raw_fd.h"
#include "model/FDTrees/fd_tree_vertex.h"

namespace model {

/**
 * FD prefix tree.
 *
 * Provides global tree manipulation and traversing methods.
 *
 * @see FDTreeVertex
 */

class FDTree {
private:
    std::shared_ptr<FDTreeVertex> root_;

    using LhsPair = std::pair<std::shared_ptr<FDTreeVertex>, boost::dynamic_bitset<>>;

public:
    explicit FDTree(size_t num_attributes) : root_(std::make_shared<FDTreeVertex>(num_attributes)) {
        for (size_t id = 0; id < num_attributes; id++) {
            root_->SetFd(id);
        }
    }

    [[nodiscard]] size_t GetNumAttributes() const noexcept {
        return root_->GetNumAttributes();
    }

    std::shared_ptr<FDTreeVertex> GetRootPtr() noexcept {
        return root_;
    }

    [[nodiscard]] FDTreeVertex const& GetRoot() const noexcept {
        return *root_;
    }

    std::shared_ptr<FDTreeVertex> AddFD(boost::dynamic_bitset<> const& lhs, size_t rhs) {
        FDTreeVertex* cur_node = root_.get();
        cur_node->SetAttribute(rhs);

        for (size_t bit = lhs.find_first(); bit != boost::dynamic_bitset<>::npos;
             bit = lhs.find_next(bit)) {
            bool is_new = cur_node->AddChild(bit);

            if (is_new && lhs.find_next(bit) == boost::dynamic_bitset<>::npos) {
                auto added_node = cur_node->GetChildPtr(bit);
                added_node->SetAttribute(rhs);
                added_node->SetFd(rhs);
                return added_node;
            }

            cur_node = cur_node->GetChild(bit);
            cur_node->SetAttribute(rhs);
        }
        cur_node->SetFd(rhs);
        return nullptr;
    }

    bool ContainsFD(boost::dynamic_bitset<> const& lhs, size_t rhs) {
        FDTreeVertex const* cur_node = root_.get();

        for (size_t bit = lhs.find_first(); bit != boost::dynamic_bitset<>::npos;
             bit = lhs.find_next(bit)) {
            if (!cur_node->HasChildren() || !cur_node->ContainsChildAt(bit)) {
                return false;
            }

            cur_node = cur_node->GetChild(bit);
        }

        return cur_node->IsFd(rhs);
    }

    /**
     * Recursively finds node representing given lhs and removes given rhs bit from it.
     * Destroys vertices whose children became empty.
     */
    void Remove(boost::dynamic_bitset<> const& lhs, size_t rhs) {
        root_->RemoveRecursive(lhs, rhs, lhs.find_first());
    }

    /**
     * Gets LHSs of all FDs having at least given lhs and rhs.
     */
    [[nodiscard]] std::vector<boost::dynamic_bitset<>> GetFdAndGenerals(
            boost::dynamic_bitset<> const& lhs, size_t rhs) const {
        assert(lhs.count() != 0);

        std::vector<boost::dynamic_bitset<>> result;
        boost::dynamic_bitset const empty_lhs(GetNumAttributes());
        size_t const starting_bit = lhs.find_first();

        root_->GetFdAndGeneralsRecursive(lhs, empty_lhs, rhs, starting_bit, result);

        return result;
    }

    /**
     * Gets LHSs of all FDs having a proper subset of giving lhs and rhs.
     */
    std::vector<boost::dynamic_bitset<>> GetGenerals(boost::dynamic_bitset<> const& lhs,
                                                     size_t rhs) {
        assert(lhs.count() != 0);

        std::vector<boost::dynamic_bitset<>> result;
        boost::dynamic_bitset empty_lhs(GetNumAttributes());
        size_t const starting_bit = lhs.find_first();

        root_->GetGeneralsRecursive(lhs, empty_lhs, rhs, starting_bit, result);

        return result;
    }

    void RemoveGenerals(boost::dynamic_bitset<> const& lhs, size_t rhs) {
        assert(lhs.count() != 0);

        boost::dynamic_bitset<> empty_lhs(GetNumAttributes());
        root_->RemoveGeneralsRecursive(lhs, empty_lhs, rhs, lhs.find_first());
    }

    std::vector<boost::dynamic_bitset<>> GetFdAndSpecials(boost::dynamic_bitset<> const& lhs,
                                                          size_t rhs) {
        std::vector<boost::dynamic_bitset<>> result;
        boost::dynamic_bitset empty_lhs(GetNumAttributes());

        root_->GetFdAndSpecialsRecursive(lhs, empty_lhs, rhs, 0, result);

        return result;
    }

    /**
     * Gets LHSs of all FDs having given lhs as a proper subset and rhs.
     */
    std::vector<boost::dynamic_bitset<>> GetSpecials(boost::dynamic_bitset<> const& lhs,
                                                     size_t rhs) {
        std::vector<boost::dynamic_bitset<>> result;
        boost::dynamic_bitset empty_lhs(GetNumAttributes());

        root_->GetSpecialsRecursive(lhs, empty_lhs, rhs, 0, result);

        return result;
    }

    void RemoveSpecials(boost::dynamic_bitset<> const& lhs, size_t rhs) {
        boost::dynamic_bitset empty_lhs(GetNumAttributes());
        root_->RemoveSpecialsRecursive(lhs, empty_lhs, rhs, 0);
    }

    /**
     * Checks if any FD has at least given lhs and rhs.
     */
    [[nodiscard]] bool ContainsFdOrGeneral(boost::dynamic_bitset<> const& lhs, size_t rhs) const {
        return root_->ContainsFdOrGeneralRecursive(lhs, rhs, lhs.find_first());
    }

    [[nodiscard]] bool ContainsFdOrSpecial(boost::dynamic_bitset<> const& lhs, size_t rhs) const {
        size_t next_after_last_lhs_set_bit = 0;
        if (lhs.find_first() != boost::dynamic_bitset<>::npos) {
            next_after_last_lhs_set_bit = lhs.find_first();
            while (lhs.find_next(next_after_last_lhs_set_bit) != boost::dynamic_bitset<>::npos) {
                next_after_last_lhs_set_bit = lhs.find_next(next_after_last_lhs_set_bit);
            }
            ++next_after_last_lhs_set_bit;
        }

        return root_->ContainsFdOrSpecialRecursive(lhs, rhs, next_after_last_lhs_set_bit, 0);
    }

    /**
     * Gets nodes representing FDs with LHS of given arity.
     * @param target_level arity of returned FDs LHSs
     */
    std::vector<LhsPair> GetLevel(unsigned target_level) {
        boost::dynamic_bitset const empty_lhs(GetNumAttributes());

        std::vector<LhsPair> vertices;
        root_->GetLevelRecursive(target_level, 0, empty_lhs, vertices);
        return vertices;
    }

    /**
     * @return vector of all FDs
     */
    [[nodiscard]] std::vector<RawFD> FillFDs() const {
        std::vector<RawFD> result;
        boost::dynamic_bitset<> lhs_for_traverse(GetRoot().GetNumAttributes());
        GetRoot().FillFDs(result, lhs_for_traverse);
        return result;
    }
};
}  // namespace model
