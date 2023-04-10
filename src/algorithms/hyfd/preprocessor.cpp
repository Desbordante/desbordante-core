#include "preprocessor.h"

#include <algorithm>
#include <vector>

#include "util/pli_util.h"

namespace {

std::vector<size_t> SortAndGetMapping(algos::hyfd::PLIs& plis) {
    size_t id = 0;
    std::vector<std::pair<algos::hyfd::PLIs::value_type, size_t>> plis_sort_ids;
    std::transform(plis.begin(), plis.end(), std::back_inserter(plis_sort_ids),
                   [&id](auto& pli) { return std::make_pair(std::move(pli), id++); });

    auto const kClusterQuantityDescending = [](auto const& pli1, auto const& pli2) {
        return pli1.first->GetNumCluster() > pli2.first->GetNumCluster();
    };
    std::sort(plis_sort_ids.begin(), plis_sort_ids.end(), kClusterQuantityDescending);

    std::transform(plis_sort_ids.begin(), plis_sort_ids.end(), plis.begin(),
                   [](auto& pli_ext) { return std::move(pli_ext.first); });

    std::vector<size_t> og_mapping(plis_sort_ids.size());
    std::transform(plis_sort_ids.begin(), plis_sort_ids.end(), og_mapping.begin(),
                   [](auto const& pli_ext) { return pli_ext.second; });
    return og_mapping;
}

algos::hyfd::Columns BuildInvertedPlis(algos::hyfd::PLIs const& plis) {
    algos::hyfd::Columns inverted_plis;

    for (auto const& pli : plis) {
        size_t cluster_id = 0;
        std::vector<size_t> current(pli->getRelationSize(),
                                    algos::hyfd::PLIUtil::kSingletonClusterId);
        for (const auto& cluster : pli->GetIndex()) {
            for (int value : cluster) {
                current[value] = cluster_id;
            }
            cluster_id++;
        }
        inverted_plis.push_back(std::move(current));
    }
    return inverted_plis;
}

algos::hyfd::Rows BuildRecordRepresentation(algos::hyfd::Columns const& inverted_plis) {
    size_t const num_columns = inverted_plis.size();
    size_t const num_rows = num_columns == 0 ? 0 : inverted_plis.begin()->size();

    algos::hyfd::Rows pli_records(num_rows, std::vector<size_t>(num_columns));

    for (size_t i = 0; i < num_rows; ++i) {
        for (size_t j = 0; j < num_columns; ++j) {
            pli_records[i][j] = inverted_plis[j][i];
        }
    }

    return pli_records;
}

}  // namespace

namespace algos::hyfd {

std::tuple<PLIs, Rows, std::vector<size_t>> Preprocess(ColumnLayoutRelationData* relation) {
    PLIs plis;
    std::transform(relation->GetColumnData().begin(), relation->GetColumnData().end(),
                   std::back_inserter(plis),
                   [](auto& column_data) { return column_data.GetPositionListIndex(); });

    auto og_mapping = SortAndGetMapping(plis);

    const auto inverted_plis = BuildInvertedPlis(plis);

    auto pli_records = BuildRecordRepresentation(inverted_plis);

    return std::make_tuple(std::move(plis), std::move(pli_records), std::move(og_mapping));
}

boost::dynamic_bitset<> RestoreAgreeSet(boost::dynamic_bitset<> const& as,
                                        std::vector<size_t> const& og_mapping, size_t num_cols) {
    boost::dynamic_bitset<> mapped_as(num_cols);
    for (size_t i = as.find_first(); i != boost::dynamic_bitset<>::npos; i = as.find_next(i)) {
        mapped_as.set(og_mapping[i]);
    }
    return mapped_as;
}

}  // namespace algos::hyfd
