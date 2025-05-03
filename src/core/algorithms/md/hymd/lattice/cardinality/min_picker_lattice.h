#pragma once

#include <cstddef>  // for size_t
#include <vector>   // for vector

#include <boost/dynamic_bitset/dynamic_bitset.hpp>  // for dynamic_bitset

#include "algorithms/md/hymd/lattice/md_lattice.h"       // for MdLattice
#include "algorithms/md/hymd/lattice/node_base.h"        // for NodeBase
#include "algorithms/md/hymd/lattice/validation_info.h"  // for ValidationInfo
#include "md/hymd/column_classifier_value_id.h"          // for ColumnClassi...
#include "md/hymd/md_lhs.h"                              // for MdLhs
#include "model/index.h"                                 // for Index

namespace algos::hymd::lattice::cardinality {

class MinPickerLattice {
private:
    struct Node : NodeBase<Node> {
        ValidationInfo* task_info = nullptr;

        Node* AddOneUnchecked(model::Index offset, ColumnClassifierValueId ccv_id) {
            return AddOneUncheckedBase(offset, ccv_id);
        }

        Node(std::size_t children_number) : NodeBase<Node>(children_number) {}
    };

    using CCVIdChildMap = Node::OrderedCCVIdChildMap;
    using NodeChildren = Node::Children;

    Node root_;
    std::vector<ValidationInfo> info_;

    static auto SetInfoAction(ValidationInfo* info) {
        return [info](Node* node) { node->task_info = info; };
    }

    void AddNewLhs(Node& cur_node, ValidationInfo* validation_info, MdLhs::iterator cur_lhs_iter);
    void ExcludeGeneralizationRhs(Node const& cur_node,
                                  MdLattice::MdVerificationMessenger const& messenger,
                                  MdLhs::iterator cur_lhs_iter,
                                  boost::dynamic_bitset<>& considered_indices);
    void RemoveSpecializations(Node& cur_node, MdLattice::MdVerificationMessenger const& messenger,
                               MdLhs::iterator cur_lhs_iter,
                               boost::dynamic_bitset<> const& picked_indices);
    void GetAll(Node& cur_node, std::vector<ValidationInfo>& collected);
    void Add(ValidationInfo* validation_info);

public:
    static constexpr bool kNeedsEmptyRemoval = false;

    void NewBatch(std::size_t max_elements);
    void AddGeneralizations(MdLattice::MdVerificationMessenger& messenger,
                            boost::dynamic_bitset<>& considered_indices);
    std::vector<ValidationInfo> GetAll() noexcept(kNeedsEmptyRemoval);

    MinPickerLattice(std::size_t col_matches_num) : root_(col_matches_num) {}
};

}  // namespace algos::hymd::lattice::cardinality
