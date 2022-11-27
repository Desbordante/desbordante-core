#pragma once

// see ../algorithms/cfd/LICENSE

#include "cfd_types.h"

typedef std::pair<Itemset, int> CFD;
typedef std::vector<CFD> CFDList;
[[maybe_unused]] bool IsValid(const Itemset& lhs, int rhs);
