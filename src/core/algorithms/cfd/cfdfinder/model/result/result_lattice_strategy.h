#pragma once
#include <list>

#include "core/algorithms/cfd/cfdfinder/model/result/result_lattice.h"
#include "core/algorithms/cfd/cfdfinder/model/result/result_strategy.h"

namespace algos::cfdfinder {
class ResultLatticeStrategy : public ResultStrategy {
private:
    std::list<ResultLattice> lattices_;

public:
    ResultLatticeStrategy() = default;

    void ReceiveResult(Candidate embedded_fd, PatternTableau tableau) override {
        RawCFD result{std::move(embedded_fd), std::move(tableau)};
        for (auto& lattice : lattices_) {
            if (lattice.CanInsert(result)) {
                lattice.Insert(std::move(result));
                return;
            }
        }
        lattices_.emplace_back(std::move(result));
    }

    std::list<RawCFD> TakeAllResults() override {
        std::list<RawCFD> results;
        for (auto& lattice : lattices_) {
            results.splice(results.end(), lattice.GetLeaves());
        }
        return results;
    }
};
}  // namespace algos::cfdfinder
