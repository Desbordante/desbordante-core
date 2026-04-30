#pragma once

#include <string>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/fd/fdep/fd_tree_element.h"
#include "core/algorithms/fd/table_mask_pair_fd_view.h"
#include "core/config/equal_nulls/type.h"
#include "core/config/max_lhs/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/relation_data.h"
#include "core/model/table/relational_schema.h"
#include "core/model/table/table_header.h"
#include "core/model/types/bitset.h"

namespace algos::fd {

class FDep final : public Algorithm {
public:
    FDep();

    ~FDep() final = default;

    TableMaskPairFdView::OwningPointer GetFds() {
        return fd_view_;
    }

private:
    TableMaskPairFdView::OwningPointer fd_view_;

    config::InputTable input_table_;
    config::MaxLhsType max_lhs_;

    model::TableHeader table_header_;

    std::unique_ptr<FDTreeElement> neg_cover_tree_{};
    std::unique_ptr<FDTreeElement> pos_cover_tree_{};

    std::vector<std::vector<size_t>> tuples_;

    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;

    void LoadDataInternal() final;

    void ResetState() final;
    unsigned long long ExecuteInternal() final;

    // Building negative cover via violated dependencies
    void BuildNegativeCover();

    // Iterating over all pairs t1 and t2 of the relation
    // Adding violated FDs to negative cover tree.
    void AddViolatedFDs(std::vector<size_t> const& t1, std::vector<size_t> const& t2);

    // Converting negative cover tree into positive cover tree
    void CalculatePositiveCover(FDTreeElement const& neg_cover_subtree,
                                model::Bitset<FDTreeElement::kMaxAttrNum>& active_path);

    // Specializing general dependencies for not to be followed from violated dependencies of
    // negative cover tree.
    void SpecializePositiveCover(model::Bitset<FDTreeElement::kMaxAttrNum> const& lhs,
                                 size_t const& a);
};

}  // namespace algos::fd
