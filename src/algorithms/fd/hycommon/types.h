#include <memory>
#include <utility>
#include <vector>

namespace structures {

class PositionListIndex;

}

namespace algos::hy {

// Row (or column) position in the table
using TablePos = unsigned int;
using ClusterId = unsigned int;

// Represents a relation as a list of position list indexes. i-th PLI is a PLI built on i-th column
// of the relation
using PLIs = std::vector<structures::PositionListIndex*>;
using PLIsPtr = std::shared_ptr<PLIs>;
using Row = std::vector<TablePos>;
// Represents a relation as a list of rows where each row is a list of row values
using Rows = std::vector<Row>;
// Represents a relation as a list of column where each column is a list of column values
using Columns = std::vector<std::vector<TablePos>>;
using RowsPtr = std::shared_ptr<Rows>;
// Pair of row numbers
using IdPairs = std::vector<std::pair<TablePos, TablePos>>;

}  // namespace algos::hy
