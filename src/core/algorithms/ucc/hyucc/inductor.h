#pragma once

#include "algorithms/ucc/hyucc/model/non_ucc_list.h"  // for NonUCCList
#include "algorithms/ucc/raw_ucc.h"                   // for RawUCC

namespace algos {
namespace hyucc {
class UCCTree;
}
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
