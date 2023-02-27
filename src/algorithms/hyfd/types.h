#include <memory>
#include <utility>
#include <vector>

namespace util {

class PositionListIndex;

}

namespace algos::hyfd {

using PLIs = std::vector<util::PositionListIndex*>;
using PLIsPtr = std::shared_ptr<PLIs>;
using Rows = std::vector<std::vector<size_t>>;
using Columns = std::vector<std::vector<size_t>>;
using RowsPtr = std::shared_ptr<Rows>;
using IdPairs = std::vector<std::pair<size_t, size_t>>;

}  // namespace algos::hyfd
