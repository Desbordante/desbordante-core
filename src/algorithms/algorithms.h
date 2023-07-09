#pragma once

/* Functional dependency mining algorithms */
#include "algorithms/fd/aidfd/aid.h"
#include "algorithms/fd/depminer/depminer.h"
#include "algorithms/fd/dfd/dfd.h"
#include "algorithms/fd/fastfds.h"
#include "algorithms/fd/fd_mine.h"
#include "algorithms/fd/fdep/fdep.h"
#include "algorithms/fd/fun.h"
#include "algorithms/fd/hyfd/hyfd.h"
#include "algorithms/fd/pyro/pyro.h"
#include "algorithms/fd/tane/tane.h"
#include "algorithms/statistics/data_stats.h"

/*Association rule mining algorithms */
#include "algorithms/association_rules/apriori.h"

/* Metric FD verifier */
#include "algorithms/metric/metric_verifier.h"

/* FD verifier */
#include "algorithms/fd/fd_verifier/fd_verifier.h"

/* Unique Column Combination mining algorithms */
#include "algorithms/fd/ucc/hyucc/hyucc.h"

/* CFD mining algorithms */
#include "algorithms/fd/cfd/fd_first_algorithm.h"

/* Algebraic constraints*/
#include "algorithms/algebraic_constraints/ac_algorithm.h"
