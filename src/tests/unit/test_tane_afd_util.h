#pragma once

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/tane/tane.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {

inline constexpr config::ErrorType kTaneTolerance = 1e-10;

inline std::unique_ptr<algos::Tane> RunTane(algos::AfdErrorMeasure measure,
                                            config::ErrorType max_error,
                                            config::MaxLhsType max_lhs,
                                            CSVConfig const& csv_config) {
    namespace onam = config::names;
    algos::StdParamsMap params{{onam::kCsvConfig, csv_config},
                               {onam::kError, max_error},
                               {onam::kAfdErrorMeasure, measure},
                               {onam::kMaximumLhs, max_lhs}};
    auto tane = algos::CreateAndLoadAlgorithm<algos::Tane>(params);
    tane->Execute();
    return tane;
}

inline ::AFD const* FindAfd(algos::Tane const& tane, std::vector<model::ColumnIndex> const& lhs,
                            model::ColumnIndex rhs) {
    for (::AFD const& afd : tane.AfdList()) {
        if (afd.GetLhsIndices() == lhs && afd.GetRhsIndex() == rhs) return &afd;
    }
    return nullptr;
}

}  // namespace tests
