#include <memory>
#include <vector>

namespace util {

class PositionListIndex;

}

namespace algos::hyfd {

using PLIs = std::vector<std::shared_ptr<util::PositionListIndex>>;
using Rows = std::vector<std::vector<size_t>>;
using Columns = Rows;
using RowsPtr = std::shared_ptr<Rows>;
using IdPairs = std::vector<std::pair<size_t, size_t>>;

}  // namespace algos::hyfd
