#include "afd_measures.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

#include "error/type.h"
#include "table/column_data.h"
#include "table/position_list_index.h"

namespace algos {
using Cluster = model::PositionListIndex::Cluster;

config::ErrorType CalculateZeroAryG1(ColumnData const* rhs, unsigned long long num_tuple_pairs) {
    return 1 - rhs->GetPositionListIndex()->GetNepAsLong() /
                       static_cast<config::ErrorType>(num_tuple_pairs);
}

config::ErrorType CalculateG1Error(model::PositionListIndex const* lhs_pli,
                                   model::PositionListIndex const* joint_pli,
                                   unsigned long long num_tuple_pairs) {
    return static_cast<config::ErrorType>((lhs_pli->GetNepAsLong() - joint_pli->GetNepAsLong()) /
                                          static_cast<config::ErrorType>(num_tuple_pairs));
}

config::ErrorType PdepSelf(model::PositionListIndex const* x_pli) {
    size_t n = x_pli->GetRelationSize();
    config::ErrorType sum = 0;
    std::size_t cluster_rows_count = 0;
    std::deque<Cluster> const& x_index = x_pli->GetIndex();
    for (Cluster const& x_cluster : x_index) {
        cluster_rows_count += x_cluster.size();
        sum += x_cluster.size() * x_cluster.size();
    }
    std::size_t unique_rows = x_pli->GetRelationSize() - cluster_rows_count;
    sum += unique_rows;
    return static_cast<config::ErrorType>(sum / (n * n));
}

config::ErrorType CalculatePdepMeasure(model::PositionListIndex const* x_pli,
                                       model::PositionListIndex const* xa_pli) {
    std::deque<Cluster> xa_index = xa_pli->GetIndex();
    std::deque<Cluster> x_index = x_pli->GetIndex();
    size_t n = x_pli->GetRelationSize();

    config::ErrorType sum = 0;

    std::unordered_map<int, size_t> x_frequencies;

    int x_value_id = 1;
    for (Cluster const& x_cluster : x_index) {
        x_frequencies[x_value_id++] = x_cluster.size();
    }

    x_frequencies[model::PositionListIndex::kSingletonValueId] = 1;

    auto x_prob = x_pli->CalculateAndGetProbingTable();

    auto get_x_freq_by_tuple_ind{[&x_prob, &x_frequencies](int tuple_ind) {
        int value_id = x_prob->at(tuple_ind);
        return static_cast<config::ErrorType>(x_frequencies[value_id]);
    }};

    for (Cluster const& xa_cluster : xa_index) {
        config::ErrorType num = xa_cluster.size() * xa_cluster.size();
        config::ErrorType denum = get_x_freq_by_tuple_ind(xa_cluster.front());
        sum += num / denum;
    }

    auto xa_prob = xa_pli->CalculateAndGetProbingTable();
    for (size_t i = 0; i < xa_prob->size(); i++) {
        if (xa_prob->at(i) == 0) {
            sum += 1 / get_x_freq_by_tuple_ind(i);
        }
    }
    return (sum / static_cast<config::ErrorType>(n));
}

config::ErrorType CalculateTauMeasure(model::PositionListIndex const* x_pli,
                                      model::PositionListIndex const* a_pli,
                                      model::PositionListIndex const* xa_pli) {
    config::ErrorType pdep_y = PdepSelf(a_pli);
    if (pdep_y == 1) return 1;

    config::ErrorType pdep_xy = CalculatePdepMeasure(x_pli, xa_pli);

    return ((pdep_xy - pdep_y) / (1 - pdep_y));
}

config::ErrorType CalculateMuPlusMeasure(model::PositionListIndex const* x_pli,
                                         model::PositionListIndex const* a_pli,
                                         model::PositionListIndex const* xa_pli) {
    config::ErrorType pdep_y = PdepSelf(a_pli);
    if (pdep_y == 1) return 1;

    config::ErrorType pdep_xy = CalculatePdepMeasure(x_pli, xa_pli);

    size_t n = x_pli->GetRelationSize();
    std::size_t cluster_rows_count = 0;
    std::deque<Cluster> const& x_index = x_pli->GetIndex();
    size_t k = x_index.size();

    for (Cluster const& x_cluster : x_index) {
        cluster_rows_count += x_cluster.size();
    }

    std::size_t unique_rows = x_pli->GetRelationSize() - cluster_rows_count;
    k += unique_rows;

    if (k == n) return 1;

    config::ErrorType mu = 1 - (1 - pdep_xy) / (1 - pdep_y) * (n - 1) / (n - k);
    config::ErrorType mu_plus = std::max(0., mu);
    return mu_plus;
}

config::ErrorType CalculateRhoMeasure(model::PositionListIndex const* x_pli,
                                      model::PositionListIndex const* xa_pli) {
    auto calculate_dom = [](model::PositionListIndex const* pli) {
        auto index = pli->GetIndex();
        size_t dom = index.size();

        std::size_t cluster_rows_count = 0;
        for (Cluster const& cluster : index) {
            cluster_rows_count += cluster.size();
        }

        std::size_t unique_rows = pli->GetRelationSize() - cluster_rows_count;
        dom += unique_rows;
        return static_cast<config::ErrorType>(dom);
    };

    config::ErrorType dom_x = calculate_dom(x_pli);
    config::ErrorType dom_xa = calculate_dom(xa_pli);
    return dom_x / dom_xa;
}
}  // namespace algos