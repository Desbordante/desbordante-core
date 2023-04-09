#include <memory>
#include <utility>
#include <vector>

namespace util {

class PositionListIndex;

}

namespace algos::hyfd {

// Represents a relation as a list of position list indexes. i-th PLI is a PLI built on i-th column
// of the relation
using PLIs = std::vector<util::PositionListIndex*>;
using PLIsPtr = std::shared_ptr<PLIs>;
// Represents a relation as a list of rows where each row is a list of row values
using Rows = std::vector<std::vector<size_t>>;
// Represents a relation as a list of column where each column is a list of column values
using Columns = std::vector<std::vector<size_t>>;
using RowsPtr = std::shared_ptr<Rows>;
// Pair of row numbers
using IdPairs = std::vector<std::pair<size_t, size_t>>;

}  // namespace algos::hyfd
