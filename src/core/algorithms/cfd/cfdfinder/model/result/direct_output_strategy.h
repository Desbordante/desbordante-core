#pragma once

#include "result_strategy.h"

namespace algos::cfdfinder {
class DirectOutputStrategy : public ResultStrategy {
private:
    std::list<RawCFD> cfd_collection_;

public:
    DirectOutputStrategy() = default;

    void ReceiveResult(Candidate embedded_fd, PatternTableau tableau) override {
        cfd_collection_.emplace_back(RawCFD{std::move(embedded_fd), std::move(tableau)});
    }

    std::list<RawCFD> TakeAllResults() override {
        return std::move(cfd_collection_);
    }
};
}  // namespace algos::cfdfinder