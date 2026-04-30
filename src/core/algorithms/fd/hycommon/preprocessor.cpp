#include "core/algorithms/fd/hycommon/preprocessor.h"

#include <algorithm>
#include <vector>

#include "core/algorithms/fd/hycommon/util/pli_util.h"
#include "core/model/index.h"
#include "core/model/table/position_list_index.h"
#include "core/model/table/create_stripped_partitions.h"
#include "core/util/logger.h"

namespace algos::hy::util {

std::vector<ClusterId> SortAndGetMapping(PLIs& plis) {
    ClusterId id = 0;
    std::vector<std::pair<PLIs::value_type, ClusterId>> plis_sort_ids;
    std::transform(plis.begin(), plis.end(), std::back_inserter(plis_sort_ids),
                   [&id](auto& pli) { return std::make_pair(std::move(pli), id++); });

    auto const cluster_quantity_descending = [](auto const& pli1, auto const& pli2) {
        return pli1.first.GetNumCluster() > pli2.first.GetNumCluster();
    };
    std::sort(plis_sort_ids.begin(), plis_sort_ids.end(), cluster_quantity_descending);

    std::transform(plis_sort_ids.begin(), plis_sort_ids.end(), plis.begin(),
                   [](auto& pli_ext) { return std::move(pli_ext.first); });

    std::vector<ClusterId> og_mapping(plis_sort_ids.size());
    std::transform(plis_sort_ids.begin(), plis_sort_ids.end(), og_mapping.begin(),
                   [](auto const& pli_ext) { return pli_ext.second; });
    return og_mapping;
}

Columns BuildInvertedPlis(PLIs const& plis) {
    Columns inverted_plis;

    for (auto const& pli : plis) {
        ClusterId cluster_id = 0;
        std::vector<ClusterId> current(pli.GetRelationSize(), PLIUtil::kSingletonClusterId);
        for (auto const& cluster : pli.GetIndex()) {
            for (int value : cluster) {
                current[value] = cluster_id;
            }
            cluster_id++;
        }
        inverted_plis.push_back(std::move(current));
    }
    return inverted_plis;
}

Rows BuildRecordRepresentation(algos::hy::Columns const& inverted_plis) {
    size_t const num_columns = inverted_plis.size();
    size_t const num_rows = num_columns == 0 ? 0 : inverted_plis.begin()->size();

    Rows pli_records(num_rows, Row(num_columns));

    for (size_t i = 0; i < num_rows; ++i) {
        for (size_t j = 0; j < num_columns; ++j) {
            pli_records[i][j] = inverted_plis[j][i];
        }
    }

    return pli_records;
}

PLIs BuildPLIs(model::IDatasetStream& data_stream) {
    PLIs plis;

    std::unordered_map<std::string, std::size_t> value_id_map;
    std::size_t const num_columns = data_stream.GetNumberOfColumns();
    auto value_id_mapped_table = std::vector<std::vector<int>>(num_columns);
    std::vector<std::string> row;
    int next_value_id = 0;

    while (data_stream.HasNextRow()) {
        row = data_stream.GetNextRow();

        if (row.size() != num_columns) {
            LOG_WARN("Unexpected number of columns for a row, skipping (expected {}, got {})",
                     num_columns, row.size());
            continue;
        }

        for (model::Index column_index = 0; column_index != num_columns; ++column_index) {
            std::string const& attribute_value = row[column_index];
            auto [it, is_new] = value_id_map.try_emplace(attribute_value, next_value_id);
            if (is_new) ++next_value_id;
            value_id_mapped_table[column_index].push_back(it->second);
        }
    }

    return plis;
}

}  // namespace algos::hy::util

namespace algos::hy {
using namespace util;

std::tuple<std::shared_ptr<PLIs>, std::shared_ptr<Rows>, std::vector<ClusterId>> Preprocess(
        model::IDatasetStream& data_stream) {
    PLIs plis = ::util::CreateStrippedPartitions(data_stream);

    auto og_mapping = SortAndGetMapping(plis);

    auto const inverted_plis = BuildInvertedPlis(plis);

    auto pli_records = BuildRecordRepresentation(inverted_plis);

    return std::make_tuple(std::make_shared<PLIs>(std::move(plis)),
                           std::make_shared<Rows>(std::move(pli_records)), std::move(og_mapping));
}

boost::dynamic_bitset<> RestoreAgreeSet(boost::dynamic_bitset<> const& as,
                                        std::vector<ClusterId> const& og_mapping, size_t num_cols) {
    boost::dynamic_bitset<> mapped_as(num_cols);
    for (size_t i = as.find_first(); i != boost::dynamic_bitset<>::npos; i = as.find_next(i)) {
        mapped_as.set(og_mapping[i]);
    }
    return mapped_as;
}

}  // namespace algos::hy
