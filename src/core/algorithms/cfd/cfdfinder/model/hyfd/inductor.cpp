#include "core/algorithms/cfd/cfdfinder/model/hyfd/inductor.h"

#include "core/util/bitset_utils.h"

namespace algos::cfdfinder {

void Inductor::UpdateFdTree(NonFDList const& non_fds) {
    unsigned const max_level = non_fds.GetDepth();

    for (unsigned level = max_level; level != 0; --level) {
        for (auto const& lhs_bits : non_fds.GetLevel(level)) {
            auto rhs_bits = lhs_bits;
            rhs_bits.flip();
            util::ForEachIndex(rhs_bits, [this, &lhs_bits](size_t rhs_id) {
                SpecializeTreeForNonFd(lhs_bits, rhs_id);
            });
        }
    }
}

void Inductor::SpecializeTreeForNonFd(boost::dynamic_bitset<> const& lhs_bits, size_t rhs_id) {
    auto invalid_lhss = tree_->GetFdAndGenerals(lhs_bits, rhs_id);

    if (invalid_lhss.empty()) {
        return;
    }

    for (auto& invalid_lhs_bits : invalid_lhss) {
        tree_->Remove(invalid_lhs_bits, rhs_id);

        for (size_t i = 0; i < tree_->GetNumAttributes(); ++i) {
            if (i == rhs_id || lhs_bits.test(i)) {
                continue;
            }

            invalid_lhs_bits.set(i);

            if (tree_->FindFdOrGeneral(invalid_lhs_bits, rhs_id)) {
                invalid_lhs_bits.reset(i);
                continue;
            }

            tree_->AddFD(invalid_lhs_bits, rhs_id);
            invalid_lhs_bits.reset(i);
        }
    }
    if (lhs_bits.any()) {
        max_non_fds_.emplace_back(lhs_bits, rhs_id);
    }
}

}  // namespace algos::cfdfinder
