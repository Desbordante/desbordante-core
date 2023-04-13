#pragma once

// see ../algorithms/cfd/LICENSE

#include "model/cfd_types.h"

typedef std::pair<Itemset, int> CFD;
typedef std::vector<CFD> CFDList;
bool IsValid(const Itemset& lhs, int rhs);
