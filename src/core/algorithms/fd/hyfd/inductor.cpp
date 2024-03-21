#include "inductor.h"

#include <boost/dynamic_bitset.hpp>

namespace algos::hyfd {

void Inductor::UpdateFdTree(NonFDList&& non_fds) {
    unsigned const max_level = non_fds.GetDepth();

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

}  // namespace algos::hyfd
