#pragma once

#include <boost/dynamic_bitset.hpp>

#include "structures/non_ucc_list.h"
#include "structures/ucc_tree.h"

namespace algos {

class Inductor {
private:
    UCCTree* tree_;

    void SpecializeUCCTree(boost::dynamic_bitset<> const& non_ucc);

public:
    explicit Inductor(UCCTree* tree) noexcept : tree_(tree) {}

    void UpdateUCCTree(NonUCCList&& non_uccs);
};

}  // namespace algos
