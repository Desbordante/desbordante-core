#include "apriori_candidate_generator.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <unordered_set>
#include <utility>

#include "ind/faida/util/simple_cc.h"
#include "ind/faida/util/simple_ind.h"

namespace algos::faida::apriori_candidate_generator {

std::shared_ptr<SimpleCC> CombineCCs(SimpleCC const& first, SimpleCC const& second,
                                     std::unordered_set<std::shared_ptr<SimpleCC>>& ccs_on_level) {
    std::vector<ColumnIndex> new_columns = first.GetColumnIndices();
    new_columns.push_back(second.GetLastColumn());
    auto [iter, was_inserted] =
            ccs_on_level.emplace(std::make_shared<SimpleCC>(first.GetTableIndex(), new_columns));
    return *iter;
}

bool CreateCandidate(SimpleIND const& first, SimpleIND const& second,
                     std::unordered_set<std::shared_ptr<SimpleCC>>& ccs_on_level,
                     std::unordered_set<SimpleIND const*> const& last_result_set,
                     std::vector<SimpleIND>& candidates) {
    std::shared_ptr<SimpleCC> new_left =
            CombineCCs(*(first.Left()), *(second.Left()), ccs_on_level);
    std::shared_ptr<SimpleCC> new_right =
            CombineCCs(*(first.Right()), *(second.Right()), ccs_on_level);

    int const size = new_left->GetColumnIndices().size();

    if (SimpleCC::HaveIndicesIntersection(*new_left, *new_right)) {
        return false;
    }

    int const num_checks = size - 2;
    if (num_checks <= 0) {
        candidates.emplace_back(std::move(new_left), std::move(new_right));
        return true;
    }

    std::vector<ColumnIndex> test_left;
    std::vector<ColumnIndex> test_right;
    test_left.reserve(size - 1);
    test_right.reserve(size - 1);

    for (ColumnIndex check = 0; check < static_cast<ColumnIndex>(num_checks); check++) {
        test_left.clear();
        test_right.clear();
        for (ColumnIndex col_idx = 0; col_idx < static_cast<ColumnIndex>(size); col_idx++) {
            if (col_idx != check) {
                test_left.push_back(new_left->GetColumnIndices()[col_idx]);
                test_right.push_back(new_right->GetColumnIndices()[col_idx]);
            }
        }
        SimpleIND const qwe =
                SimpleIND(std::make_shared<SimpleCC>(new_left->GetTableIndex(), test_left),
                          std::make_shared<SimpleCC>(new_right->GetTableIndex(), test_right));
        if (last_result_set.find(&qwe) == last_result_set.end()) {
            return false;
        }
    }

    candidates.emplace_back(std::move(new_left), std::move(new_right));
    return true;
}

std::vector<SimpleIND> CreateCombinedCandidates(std::vector<SimpleIND> const& inds) {
    std::vector<SimpleIND const*> last_result(inds.size());
    std::transform(inds.begin(), inds.end(), last_result.begin(),
                   [](SimpleIND const& i) { return &i; });
    std::sort(last_result.begin(), last_result.end(),
              [](SimpleIND const* a, SimpleIND const* b) { return *a < *b; });

    std::unordered_set<SimpleIND const*> last_result_set(last_result.begin(), last_result.end());

    std::unordered_set<std::shared_ptr<SimpleCC>> ccs_on_level;
    std::vector<SimpleIND> candidates;
    for (auto left_iter = last_result.begin(); left_iter != last_result.end(); left_iter++) {
        for (auto right_iter = std::next(left_iter); right_iter != last_result.end();
             right_iter++) {
            SimpleIND const& first_ind = **left_iter;
            SimpleIND const& second_ind = **right_iter;

            if (!first_ind.StartsWith(second_ind)) {
                break;
            }

            if (first_ind.Left()->GetLastColumn() == second_ind.Left()->GetLastColumn() ||
                first_ind.Right()->GetLastColumn() == second_ind.Right()->GetLastColumn()) {
                continue;
            }

            CreateCandidate(first_ind, second_ind, ccs_on_level, last_result_set, candidates);
        }
    }

    return candidates;
}

}  // namespace algos::faida::apriori_candidate_generator
