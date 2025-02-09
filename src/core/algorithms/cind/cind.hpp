#pragma once

#include <ind/ind.h>

#include "condition.h"

namespace algos::cind {
struct Cind {
    model::IND const& ind_;
    Condition cond_;
};
}  // namespace algos::cind