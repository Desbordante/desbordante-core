#pragma once

/* Functional dependency mining algorithms */
#include "algorithms/functional/aidfd/aid.h"
#include "algorithms/functional/depminer/depminer.h"
#include "algorithms/functional/dfd/dfd.h"
#include "algorithms/functional/fastfds.h"
#include "algorithms/functional/fd_mine.h"
#include "algorithms/functional/fdep/fdep.h"
#include "algorithms/functional/fun.h"
#include "algorithms/functional/hyfd/hyfd.h"
#include "algorithms/functional/pyro/pyro.h"
#include "algorithms/functional/tane/tane.h"
#include "algorithms/statistics/data_stats.h"

/*Association rule mining algorithms */
#include "algorithms/association_rules/apriori.h"

/* Metric FD verifier */
#include "algorithms/metric/metric_verifier.h"

/* FD verifier */
#include "algorithms/functional/fd_verifier/fd_verifier.h"

/* Unique Column Combination mining algorithms */
#include "algorithms/functional/ucc/hyucc/hyucc.h"

/* CFD mining algorithms */
#include "algorithms/functional/cfd/fd_first_algorithm.h"

/* Algebraic constraints*/
#include "algorithms/algebraic_constraints/ac_algorithm.h"
