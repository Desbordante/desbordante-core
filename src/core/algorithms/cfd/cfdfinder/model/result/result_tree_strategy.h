#pragma once

#include <limits>
#include <list>

#include "result_strategy.h"
#include "result_tree.h"

namespace algos::cfdfinder {
class ResultTreeStrategy : public ResultStrategy {
private:
    std::list<ResultTree> trees_;

public:
    ResultTreeStrategy() = default;

    void ReceiveResult(Candidate embedded_fd, PatternTableau tableau) override {
        RawCFD result{std::move(embedded_fd), std::move(tableau)};
        double min_distance = std::numeric_limits<double>{}.max();
        ResultTree* min_position = nullptr;
        for (auto& tree : trees_) {
            auto position = tree.GetInsertPosition(result);
            if (position != nullptr &&
                position->GetSupportRoot() - result.patterns_.GetSupport() < min_distance) {
                min_distance = position->GetSupportRoot() - result.patterns_.GetSupport();
                min_position = position;
            }
        }
        if (min_position != nullptr) {
            min_position->AddChild(std::move(result));
        } else {
            trees_.emplace_back(std::move(result));
        }
    }

    std::list<RawCFD> TakeAllResults() override {
        std::list<RawCFD> results;
        for (auto& tree : trees_) {
            results.splice(results.end(), tree.GetLeaves());
        }
        return results;
    }
};
}  // namespace algos::cfdfinder
