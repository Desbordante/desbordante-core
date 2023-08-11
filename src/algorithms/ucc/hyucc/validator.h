#pragma once

#include <cassert>
#include <utility>
#include <vector>

#include "algorithms/ucc/hyucc/model/ucc_tree.h"
#include "algorithms/ucc/raw_ucc.h"
#include "config/thread_number/type.h"
#include "fd/hycommon/primitive_validations.h"
#include "fd/hycommon/types.h"
#include "model/table/position_list_index.h"

namespace algos::hyucc {

class Validator {
private:
    using UCCValidations = hy::PrimitiveValidations<model::RawUCC>;

    UCCTree* tree_;
    hy::PLIsPtr plis_;
    hy::RowsPtr compressed_records_;
    unsigned current_level_number_ = 1;
    config::ThreadNumType threads_num_ = 1;

    bool IsUnique(model::PLI const& pivot_pli, model::RawUCC const& ucc,
                  hy::IdPairs& comparison_suggestions);
    UCCValidations GetValidations(LhsPair const& vertex_and_ucc);
    UCCValidations ValidateAndExtendSeq(std::vector<LhsPair> const& current_level);
    UCCValidations ValidateAndExtendParallel(std::vector<LhsPair> const& current_level);
    UCCValidations ValidateAndExtend(std::vector<LhsPair> const& current_level);

public:
    Validator(UCCTree* tree, hy::PLIsPtr plis, hy::RowsPtr compressed_records,
              config::ThreadNumType threads_num) noexcept
        : tree_(tree),
          plis_(std::move(plis)),
          compressed_records_(std::move(compressed_records)),
          threads_num_(threads_num) {}

    hy::IdPairs ValidateAndExtendCandidates();
};

}  // namespace algos::hyucc
