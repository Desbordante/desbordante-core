#pragma once

#include <utility>
#include <vector>

#include "hyfd/elements/raw_fd.h"
#include "structures/fd_tree.h"
#include "structures/non_fds.h"
#include "types.h"
#include "util/position_list_index.h"

namespace algos::hyfd {

using LhsPair = fd_tree::LhsPair;

class Validator {
private:
    struct FDValidations;

    std::shared_ptr<fd_tree::FDTree> fds_;
    std::shared_ptr<NonFds> non_fds_;

    PLIs plis_;
    RowsPtr compressed_records_;

    unsigned current_level_number_ = 0;

    FDValidations ProcessZeroLevel(LhsPair const& lhsPair);
    FDValidations ProcessFirstLevel(LhsPair const& lhsPair);
    FDValidations ProcessHigherLevel(LhsPair const& lhsPair);

    FDValidations GetValidations(LhsPair const& lhsPair);

    FDValidations ValidateSeq(std::vector<LhsPair> const& vertices);

    [[nodiscard]] unsigned GetLevelNum() const {
        return current_level_number_;
    }

public:
    Validator(std::shared_ptr<fd_tree::FDTree> fds, PLIs plis, RowsPtr compressed_records) noexcept
        : fds_(std::move(fds)),
          plis_(std::move(plis)),
          compressed_records_(std::move(compressed_records)) {}

    IdPairs Validate();
};

}  // namespace algos::hyfd
