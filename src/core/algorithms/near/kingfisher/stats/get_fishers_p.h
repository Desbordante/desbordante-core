#pragma once

#include <algorithm>

#include "algorithms/near/near.h"
#include "boost/math/distributions/hypergeometric.hpp"
#include "model/transaction/transactional_data.h"

namespace kingfisher {

double GetFishersP(model::NeARIDs const& rule,
                   std::shared_ptr<model::TransactionalData> transactional_data) {
    auto hasAnte = [&](std::vector<unsigned> const& ids, model::NeARIDs const& r) {
        for (auto f : r.ante)
            if (std::find(ids.begin(), ids.end(), f) == ids.end()) return false;
        return true;
    };
    auto hasCons = [&](std::vector<unsigned> const& ids, model::NeARIDs const& r) {
        bool found = std::find(ids.begin(), ids.end(), r.cons.feature) != ids.end();
        return r.cons.positive ? found : !found;
    };

    // Count a, b, c, d
    std::size_t a = 0, b = 0, c = 0, d = 0;
    for (auto const& [_, tx] : transactional_data->GetTransactions()) {
        bool X = hasAnte(tx.GetItemsIDs(), rule);
        bool Y = hasCons(tx.GetItemsIDs(), rule);
        if (X && Y)
            ++a;
        else if (X && !Y)
            ++b;
        else if (!X && Y)
            ++c;
        else
            ++d;
    }

    unsigned const N = a + b + c + d;
    unsigned const r = a + c;
    unsigned const n = a + b;

    using boost::math::hypergeometric_distribution;
    hypergeometric_distribution<> dist(r, n, N);

    // Find valid lower bound for the quantile
    int const support_min = std::max(0, int(n + r - N));
    int const quantile = int(a) - 1;

    if (quantile < support_min) {
        // Whole right tail = 1
        return 1.0;
    }

    return boost::math::cdf(boost::math::complement(dist, static_cast<double>(quantile)));
}

} // namespace kingfisher
