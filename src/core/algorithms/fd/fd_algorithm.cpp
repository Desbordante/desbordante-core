#include "fd_algorithm.h"

#include <map>
#include <thread>
#include <vector>

#include "config/max_lhs/option.h"

namespace algos {

FDAlgorithm::FDAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    RegisterOptions();
}

void FDAlgorithm::RegisterOptions() {
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
}

void FDAlgorithm::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kMaxLhsOpt.GetName()});
    MakeExecuteOptsAvailableFDInternal();
}

void FDAlgorithm::ResetState() {
    fd_collection_.Clear();
    ResetStateFd();
}

std::list<FD>& FDAlgorithm::SortedFdList() {
    std::list<FD>& fd_collection = fd_collection_.AsList();
    fd_collection.sort([](const FD& l_fd, const FD& r_fd) {
        if (l_fd.GetLhs().GetArity() != r_fd.GetLhs().GetArity()) {
            return l_fd.GetLhs().GetArity() < r_fd.GetLhs().GetArity();
        }
        if (l_fd.GetLhs() != r_fd.GetLhs()) {
            return l_fd.GetLhs() < r_fd.GetLhs();
        }
        return l_fd.GetRhsIndex() < r_fd.GetRhsIndex();
    });
    return fd_collection;
}

std::string FDAlgorithm::GetJsonFDs() const {
    return FDsToJson(FdList());
}

unsigned int FDAlgorithm::Fletcher16() {
    std::string to_hash = GetJsonFDs();
    unsigned int sum1 = 0, sum2 = 0, modulus = 255;
    for (auto ch : to_hash) {
        sum1 = (sum1 + ch) % modulus;
        sum2 = (sum2 + sum1) % modulus;
    }
    return (sum2 << 8) | sum1;
}

}  // namespace algos
