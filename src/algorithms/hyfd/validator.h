#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "primitive_validations.h"
#include "raw_fd.h"
#include "structures/fd_tree.h"
#include "structures/non_fds.h"
#include "types.h"
#include "util/position_list_index.h"

namespace algos::hyfd {

using LhsPair = fd_tree::LhsPair;

class Validator {
private:
    using FDValidations = PrimitiveValidations<RawFD>;

    std::shared_ptr<fd_tree::FDTree> fds_;

    PLIsPtr plis_;
    RowsPtr compressed_records_;

    unsigned current_level_number_ = 0;

    FDValidations ProcessZeroLevel(LhsPair const& lhsPair);
    FDValidations ProcessFirstLevel(LhsPair const& lhs_pair);
    FDValidations ProcessHigherLevel(LhsPair const& lhs_pair);

    FDValidations GetValidations(LhsPair const& lhsPair);

    FDValidations ValidateAndExtendSeq(std::vector<LhsPair> const& vertices);

    [[nodiscard]] unsigned GetLevelNum() const {
        return current_level_number_;
    }

    void LogLevel(const std::vector<LhsPair>& cur_level_vertices, const FDValidations& result,
                  size_t candidates) const;

public:
    Validator(std::shared_ptr<fd_tree::FDTree> fds, PLIsPtr plis,
              RowsPtr compressed_records) noexcept
        : fds_(std::move(fds)),
          plis_(std::move(plis)),
          compressed_records_(std::move(compressed_records)) {}

    IdPairs ValidateAndExtendCandidates();
};

}  // namespace algos::hyfd
