#include "core/algorithms/fd/hyfd/inductor.h"

#include <cassert>

#include <boost/dynamic_bitset.hpp>

namespace algos::fd::hyfd {

void Inductor::UpdateFdTree(NonFDList&& non_fds) {
    unsigned const max_level = non_fds.GetDepth();

    // This is not an error, tree_->GetFdAndGenerals relies on at least one bit being set.
    // Idk whether it's totally sensible, though, looks like iteration that was not thought through.
    for (unsigned level = max_level; level != 0; level--) {
        for (auto const& lhs_bits : non_fds.GetLevel(level)) {
            auto rhs_bits = lhs_bits;
            rhs_bits.flip();

            for (size_t rhs_id = rhs_bits.find_first(); rhs_id != boost::dynamic_bitset<>::npos;
                 rhs_id = rhs_bits.find_next(rhs_id)) {
                SpecializeTreeForNonFd(lhs_bits, rhs_id);
            }
        }
    }
}

void Inductor::SpecializeTreeForNonFd(boost::dynamic_bitset<> const& lhs_bits, size_t rhs_id) {
    auto invalid_lhss = tree_->GetFdAndGenerals(lhs_bits, rhs_id);

    for (auto& invalid_lhs_bits : invalid_lhss) {
        tree_->Remove(invalid_lhs_bits, rhs_id);
        if (invalid_lhs_bits.count() == max_lhs_) continue;
        assert(invalid_lhs_bits.count() < max_lhs_);

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
}

}  // namespace algos::fd::hyfd
