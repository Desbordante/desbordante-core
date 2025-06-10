#pragma once

#include <iostream>
#include <string.h>

#include "algorithms/near/kingfisher/node_adress.h"
#include "algorithms/near/kingfisher/util/vector_to_string.h"
#include "algorithms/near/types.h"
#include "get_frequency.h"

namespace kingfisher {

double GetLowerBound1(FeatureIndex feature, model::TransactionalData const* transactional_data) {
    unsigned occurences = GetItemsetOccurences({feature}, transactional_data);

    // Fast approximation of k! * (n-k)! / n!
    // For huge n (e.g., >10⁸++), numerical errors will creep in — if that’s an issue, switch to
    // cpp_dec_float. Computes k! * (n-k)! / n! using logarithms to avoid overflow
    auto lower_bound_1_factorial_relation = [](unsigned num_rows, unsigned feat_occur) -> double {
        if (feat_occur > num_rows) throw std::domain_error("k must be <= n");

        double ln_n_fact = std::lgamma(num_rows + 1.0);
        double ln_k_fact = std::lgamma(feat_occur + 1.0);
        double ln_nmk_fact = std::lgamma(num_rows - feat_occur + 1.0);

        return std::exp(ln_k_fact + ln_nmk_fact - ln_n_fact);
    };
    size_t num_rows = transactional_data->GetNumTransactions();
    return lower_bound_1_factorial_relation(num_rows, occurences);
}

double GetLowerBound2(model::NeARIDs const& near,
                      model::TransactionalData const* transactional_data) {
    unsigned ante_occur = GetItemsetOccurences(near.ante, transactional_data);
    unsigned neg_ante_occur = GetNegativeItemsetOccurences(near.ante, transactional_data);
    unsigned cons_matches = 0;
    if (near.cons.positive) {
        cons_matches = GetItemsetOccurences({near.cons.feature}, transactional_data);
    } else {
        cons_matches = GetNegativeItemsetOccurences({near.cons.feature}, transactional_data);
    }
    size_t num_rows = transactional_data->GetNumTransactions();

    auto lower_bound_2_factorial_relation = [](unsigned ante_occur, unsigned neg_ante_occur,
                                               unsigned cons_matches, unsigned num_rows) -> double {
        if (cons_matches < ante_occur) {
            throw std::invalid_argument("cons_occur must be >= ante_occur");
        }

        double log_result = std::lgamma(neg_ante_occur + 1) + std::lgamma(cons_matches + 1) -
                            std::lgamma(num_rows + 1) - std::lgamma(cons_matches - ante_occur + 1);

        return std::exp(log_result);
    };

    return lower_bound_2_factorial_relation(ante_occur, neg_ante_occur, cons_matches, num_rows);
}

double GetLowerBound3(model::NeARIDs const& near,
                      model::TransactionalData const* transactional_data) {
    unsigned cons_occur = GetItemsetOccurences({near.cons.feature}, transactional_data);
    unsigned cons_doesnt_match = 0;
    if (near.cons.positive) {
        cons_doesnt_match = GetNegativeItemsetOccurences({near.cons.feature}, transactional_data);
    } else {
        cons_doesnt_match = GetItemsetOccurences({near.cons.feature}, transactional_data);
    }
    unsigned ante_and_cons_matches = GetRuleOccurences(near, transactional_data);
    unsigned neg_ante_and_cons_matches = GetNegRuleOccurences(near, transactional_data);
    size_t num_rows = transactional_data->GetNumTransactions();

    auto lower_bound_3_factorial_relation =
            [](unsigned cons_occur, unsigned cons_doesnt_match, unsigned ante_and_cons_matches,
               unsigned neg_ante_and_cons_matches, unsigned num_rows) -> double {
        if (cons_occur > num_rows || ante_and_cons_matches > num_rows) {
            throw std::invalid_argument("cons_occur and ante_and_cons_matches must be <= num_rows");
        }

        double log_result = std::lgamma(cons_occur + 1) + std::lgamma(num_rows - cons_occur + 1) +
                            std::lgamma(num_rows - ante_and_cons_matches + 1) -
                            std::lgamma(num_rows + 1) - std::lgamma(cons_doesnt_match + 1) -
                            std::lgamma(neg_ante_and_cons_matches + 1);

        return std::exp(log_result);
    };

    return lower_bound_3_factorial_relation(cons_occur, cons_doesnt_match, ante_and_cons_matches,
                                            neg_ante_and_cons_matches, num_rows);
}

}  // namespace kingfisher
