#pragma once

#include <magic_enum/magic_enum.hpp>

#include "core/algorithms/algorithms.h"

namespace algos {

using AlgorithmTypes =
        std::tuple<Depminer, DFD, FastFDs, FDep, FdMine, Pyro, Tane, PFDTane, FUN, hyfd::HyFD, Aid,
                   EulerFD, Apriori, des::DES, metric::MetricVerifier, DataStats,
                   fd_verifier::FDVerifier, HyUCC, PyroUCC, HPIValid, cfd::FDFirstAlgorithm,
                   ACAlgorithm, UCCVerifier, Faida, Spider, Mind, INDVerifier, Fastod, GfdValidator,
                   EGfdValidator, NaiveGfdValidator, order::Order, dd::Split, Cords, hymd::HyMD,
                   PFDVerifier, cfd_verifier::CFDVerifier>;

// clang-format off
/* Enumeration of all supported non-pipeline algorithms. If you implement a new
 * algorithm please add its corresponding value to this enum and to the type
 * tuple above.
 * NOTE: algorithm string name representation is taken from the value in this
 * enum, so name it appropriately (lowercase and without additional symbols).
 */

enum class AlgorithmType : char {
/* Functional dependency mining algorithms */
    kDepminer = 0,
    kDfd,
    kFastfds,
    kFdep,
    kFdmine,
    kPyro,
    kTane,
    kPfdtane,
    kFun,
    kHyfd,
    kAidfd,
    kEulerfd,

/* Association rules mining algorithms */
    kApriori,

/* Numerical association rules mining algorithms*/
    kDes,

/* Metric verifier algorithm */
    kMetric,

/* Statistic algorithms */
    kStats,

/* FD verifier algorithm */
    kFdVerifier,

/* Unique Column Combination mining algorithms */
    kHyucc,
    kPyroucc,
    kHpivalid,

/* CFD mining algorithms */
    kFdFirstDfs,

/* Algebraic constraints mining algorithm*/
    kAc,

/* UCC verifier algorithm */
    kUccVerifier,

/* Inclusion dependency mining algorithms */
    kFaida,
    kSpider,
    kMind,

/* IND verifier algorithm */
    kIndVerifier,

/* Order dependency mining algorithms */
    kFastod,

/* Graph functional dependency mining algorithms */
    kGfdvalid,
    kEgfdvalid,
    kNaivegfdvalid,

/* Order dependency mining algorithms */
    kOrder,

/* Differential dependencies mining algorithm */
    kSplit,

/* SFD mining algorithm */
    kCords,

/* MD mining algorithms */
    kHymd,

/* PFD verifier algorithm */
    kPfdVerifier,

/* CFD verifier algorithm */
    kCfdVerifier
};
// clang-format on

static_assert(std::tuple_size_v<AlgorithmTypes> == magic_enum::enum_count<AlgorithmType>(),
              "The AlgorithmTypes tuple and the AlgorithmType enum sizes must be the same. Did you "
              "forget to add your new algorithm to either of those?");

}  // namespace algos
