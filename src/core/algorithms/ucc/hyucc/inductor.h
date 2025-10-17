#pragma once

#include <boost/dynamic_bitset.hpp>

#include "algorithms/ucc/hyucc/model/non_ucc_list.h"
#include "algorithms/ucc/hyucc/model/ucc_tree.h"
#include "algorithms/ucc/raw_ucc.h"

namespace algos {
namespace hyucc {
class UCCTree;
}  // namespace hyucc
}  // namespace algos

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
