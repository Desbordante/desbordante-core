#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "algorithms/fd/model/raw_fd.h"
#include "hycommon/primitive_validations.h"
#include "structures/fd_tree.h"
#include "structures/position_list_index.h"
#include "types.h"

namespace algos::hyfd {

using LhsPair = fd_tree::LhsPair;

class Validator {
private:
    using FDValidations = hy::PrimitiveValidations<RawFD>;

    std::shared_ptr<fd_tree::FDTree> fds_;

    hy::PLIsPtr plis_;
    hy::RowsPtr compressed_records_;

    unsigned current_level_number_ = 0;

    FDValidations ProcessZeroLevel(LhsPair const& lhsPair);
    FDValidations ProcessFirstLevel(LhsPair const& lhs_pair);
    FDValidations ProcessHigherLevel(LhsPair const& lhs_pair);

    FDValidations GetValidations(LhsPair const& lhsPair);

    FDValidations ValidateAndExtendSeq(std::vector<LhsPair> const& vertices);

    [[nodiscard]] unsigned GetLevelNum() const {
        return current_level_number_;
    }

public:
    Validator(std::shared_ptr<fd_tree::FDTree> fds, hy::PLIsPtr plis,
              hy::RowsPtr compressed_records) noexcept
        : fds_(std::move(fds)),
          plis_(std::move(plis)),
          compressed_records_(std::move(compressed_records)) {}

    hy::IdPairs ValidateAndExtendCandidates();
};

}  // namespace algos::hyfd
