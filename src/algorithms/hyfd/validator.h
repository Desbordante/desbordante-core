#pragma once

#include "structures/fd_tree.h"
#include "types.h"

namespace algos::hyfd {

class Validator {
private:
    std::shared_ptr<fd_tree::FDTree> fds_;

    PLIs plis_;
    RowsPtr compressed_records_;

public:
    Validator(std::shared_ptr<fd_tree::FDTree> fds, PLIs plis, RowsPtr compressed_records) noexcept
        : fds_(std::move(fds)),
          plis_(std::move(plis)),
          compressed_records_(std::move(compressed_records)) {}

    IdPairs Validate();
};

}  // namespace algos::hyfd
