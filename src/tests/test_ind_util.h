#pragma once
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/ind/ind.h"
#include "table_config.h"

namespace tests {

using CCTest = std::pair<model::TableIndex, std::vector<model::ColumnIndex>>;
using INDTest = std::pair<CCTest, CCTest>;

std::string TableNamesToString(std::vector<TableConfig> const& configs);
INDTest ToINDTest(model::IND const& ind);
std::vector<INDTest> ToSortedINDTestVec(std::list<model::IND> const& inds);

}  // namespace tests
