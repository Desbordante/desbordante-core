#pragma once

#include <boost/dynamic_bitset.hpp>

#include "algorithms/functional/model/raw_ucc.h"
#include "structures/non_ucc_list.h"
#include "structures/ucc_tree.h"

namespace algos::hyucc {

class Inductor {
private:
    UCCTree* tree_;

    void SpecializeUCCTree(model::RawUCC const& non_ucc);

public:
    explicit Inductor(UCCTree* tree) noexcept : tree_(tree) {}

    void UpdateUCCTree(NonUCCList&& non_uccs);
};

}  // namespace algos::hyucc
