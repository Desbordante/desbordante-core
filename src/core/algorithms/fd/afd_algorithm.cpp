#include "afd_algorithm.h"

#include <map>
#include <thread>
#include <vector>

#include "config/max_lhs/option.h"

namespace algos {

AFDAlgorithm::AFDAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    RegisterOptions();
}

void AFDAlgorithm::RegisterOptions() {
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
}

void AFDAlgorithm::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kMaxLhsOpt.GetName()});
    MakeExecuteOptsAvailableFDInternal();
}

void AFDAlgorithm::ResetState() {
    afd_collection_.Clear();
    ResetStateFd();
}

std::list<AFD>& AFDAlgorithm::SortedAfdList() {
    std::list<AFD>& afd_collection = afd_collection_.AsList();
    afd_collection.sort([](const AFD& l_fd, const AFD& r_fd) {
        if (l_fd.GetLhs().GetArity() != r_fd.GetLhs().GetArity()) {
            return l_fd.GetLhs().GetArity() < r_fd.GetLhs().GetArity();
        }
        if (l_fd.GetLhs() != r_fd.GetLhs()) {
            return l_fd.GetLhs() < r_fd.GetLhs();
        }
        return l_fd.GetRhsIndex() < r_fd.GetRhsIndex();
    });
    return afd_collection;
}

unsigned int AFDAlgorithm::Fletcher16() {
    std::string to_hash = AFDsToJson(AfdList());
    unsigned int sum1 = 0, sum2 = 0, modulus = 255;
    for (auto ch : to_hash) {
        sum1 = (sum1 + ch) % modulus;
        sum2 = (sum2 + sum1) % modulus;
    }
    return (sum2 << 8) | sum1;
}
}  // namespace algos
