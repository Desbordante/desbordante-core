#pragma once

#include "model/table/vertical.h"

namespace model {

// UCC stands for Unique Column Colmbinations and denotes a list of columns whose projection
// contains no duplicate entry. I.e. there is no tuples that have equal values in attributes
// presenting in the UCC.
// Defining UCC as Vertical, because we don't really need more functionality than Vertical provides.
using UCC = Vertical;

}  // namespace model
