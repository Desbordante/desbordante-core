#pragma once

#include <memory>   // for shared_ptr
#include <utility>  // for move
#include <vector>   // for vector

#include <boost/move/utility_core.hpp>  // for move

#include "algorithms/fd/hycommon/primitive_validations.h"  // for PrimitiveV...
#include "algorithms/fd/raw_fd.h"                          // for RawFD
#include "config/thread_number/type.h"                     // for ThreadNumType
#include "fd/hycommon/types.h"                             // for PLIsPtr
#include "fd/hyfd/model/fd_tree_vertex.h"                  // for LhsPair

namespace algos {
namespace hyfd {
namespace fd_tree {
class FDTree;
}
}  // namespace hyfd
}  // namespace algos

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

    FDValidations ValidateAndExtendPar(std::vector<LhsPair> const& vertices);

    [[nodiscard]] unsigned GetLevelNum() const {
        return current_level_number_;
    }

    config::ThreadNumType threads_num_ = 1;

public:
    Validator(std::shared_ptr<fd_tree::FDTree> fds, hy::PLIsPtr plis,
              hy::RowsPtr compressed_records, config::ThreadNumType threads_num) noexcept
        : fds_(std::move(fds)),
          plis_(std::move(plis)),
          compressed_records_(std::move(compressed_records)),
          threads_num_(threads_num) {}

    hy::IdPairs ValidateAndExtendCandidates();
};

}  // namespace algos::hyfd
