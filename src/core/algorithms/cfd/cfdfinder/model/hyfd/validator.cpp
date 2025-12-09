#include "core/algorithms/cfd/cfdfinder/model/hyfd/validator.h"

#include <algorithm>
#include <cstddef>
#include <future>
#include <unordered_set>
#include <utility>

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/hycommon/util/pli_util.h"
#include "core/algorithms/fd/hycommon/validator_helpers.h"
#include "core/algorithms/fd/hyfd/hyfd_config.h"
#include "core/util/bitset_utils.h"

namespace {
using BitSet = boost::dynamic_bitset<>;

std::unordered_set<size_t> AsSet(BitSet const& bitset) {
    std::unordered_set<size_t> valid_rhss;
    valid_rhss.reserve(bitset.count());
    util::ForEachIndex(bitset, [&valid_rhss](size_t attr) { valid_rhss.insert(attr); });

    return valid_rhss;
}

std::pair<std::vector<size_t>, std::vector<size_t>> BuildRhsMappings(
        BitSet const& rhs, algos::cfdfinder::Rows const& compressed_records) {
    std::vector<size_t> rhs_column_ids;
    rhs_column_ids.reserve(rhs.count());
    std::vector<size_t> rhs_ranks(compressed_records[0].size());

    util::ForEachIndex(rhs, [&rhs_ranks, &rhs_column_ids](size_t attr) {
        rhs_ranks[attr] = rhs_column_ids.size();
        rhs_column_ids.push_back(attr);
    });

    return {std::move(rhs_column_ids), std::move(rhs_ranks)};
}

using LhsRow = std::vector<size_t>;
using RhsRowId = std::pair<std::vector<size_t>, size_t>;

void ValidateRhss(RhsRowId const& rhs_record, algos::cfdfinder::Rows const& compressed_records,
                  size_t row, std::vector<size_t> const& rhs_ranks,
                  std::unordered_set<size_t>& valid_rhs_ids,
                  algos::hy::IdPairs& comparison_suggestions) {
    for (auto it = valid_rhs_ids.begin(); it != valid_rhs_ids.end();) {
        size_t const rhs_column = *it;
        size_t const value = compressed_records[row][rhs_column];

        if (algos::hy::PLIUtil::IsSingletonCluster(value) ||
            value != rhs_record.first[rhs_ranks[rhs_column]]) {
            comparison_suggestions.emplace_back(row, rhs_record.second);
            it = valid_rhs_ids.erase(it);
        } else {
            ++it;
        }
    }
}

RhsRowId BuildRhsRowId(algos::cfdfinder::Rows const& compressed_records, BitSet const& rhs,
                       std::vector<size_t> const& rhs_column_ids, size_t row) {
    std::vector<size_t> rhs_sub_cluster(rhs.count());
    for (size_t i = 0; i < rhs.count(); ++i) {
        rhs_sub_cluster[i] = compressed_records[row][rhs_column_ids[i]];
    }

    return {std::move(rhs_sub_cluster), row};
}

BitSet Refine(algos::hy::IdPairs& comparison_suggestions, algos::cfdfinder::PLIs const& plis,
              algos::cfdfinder::Rows const& compressed_records, BitSet const& lhs,
              BitSet const& rhs, size_t firstAttr) {
    auto valid_rhs_ids = AsSet(rhs);
    auto const lhs_column_ids = util::BitsetToIndices<algos::hy::ClusterId>(lhs);
    auto const [rhs_column_ids, rhs_ranks] = BuildRhsMappings(rhs, compressed_records);

    for (auto const& cluster : plis[firstAttr]->GetIndex()) {
        auto lhs_rhs_map = algos::hy::MakeClusterIdentifierToTMap<RhsRowId>(cluster.size());

        for (size_t row : cluster) {
            auto lhs_row =
                    algos::hy::BuildClustersIdentifier(compressed_records[row], lhs_column_ids);
            if (lhs_row.empty()) {
                continue;
            }

            auto const iter = lhs_rhs_map.find(lhs_row);

            if (iter != lhs_rhs_map.end()) {
                ValidateRhss(iter->second, compressed_records, row, rhs_ranks, valid_rhs_ids,
                             comparison_suggestions);
            } else {
                RhsRowId rhs_row_id = BuildRhsRowId(compressed_records, rhs, rhs_column_ids, row);

                lhs_rhs_map.emplace(std::move(lhs_row), std::move(rhs_row_id));
            }
        }
    }
    return util::IndicesToBitset(valid_rhs_ids, rhs.size());
}

size_t AddExtendedCandidatesFromInvalid(std::vector<algos::cfdfinder::LhsPair>& next_level,
                                        algos::hyfd::fd_tree::FDTree& fds_tree,
                                        std::vector<RawFD> const& invalid_fds,
                                        size_t num_attributes,
                                        std::list<algos::cfdfinder::Candidate>& max_non_fds) {
    size_t candidates = 0;
    for (auto const& [lhs, rhs] : invalid_fds) {
        bool found = false;
        for (size_t attr = 0; attr < num_attributes; ++attr) {
            if (lhs.test(attr) || rhs == attr || fds_tree.FindFdOrGeneral(lhs, attr) ||
                (fds_tree.GetRoot().HasChildren() && fds_tree.GetRoot().ContainsChildAt(attr) &&
                 fds_tree.GetRoot().GetChild(attr)->IsFd(rhs))) {
                continue;
            }

            BitSet lhs_ext = lhs;
            lhs_ext.set(attr);

            if (fds_tree.FindFdOrGeneral(lhs_ext, rhs)) {
                continue;
            }

            found = true;
            auto child = fds_tree.AddFD(lhs_ext, rhs);
            if (child == nullptr) {
                continue;
            }
            next_level.emplace_back(std::move(child), std::move(lhs_ext));
            ++candidates;
        }
        if (!found && lhs.any()) {
            max_non_fds.emplace_back(lhs, rhs);
        }
    }
    return candidates;
}

}  // namespace

namespace algos::cfdfinder {

Validator::FDValidations Validator::ProcessZeroLevel(LhsPair const& lhsPair) const {
    FDValidations result;

    auto vertex = lhsPair.first;
    auto const lhs = lhsPair.second;
    auto const rhs = vertex->GetFDs();
    size_t const rhs_count = rhs.count();

    result.SetCountValidations(rhs_count);
    result.SetCountIntersections(rhs_count);

    util::ForEachIndex(rhs, [&](size_t attr) {
        if (!plis_->at(attr)->IsConstant()) {
            vertex->RemoveFd(attr);
            result.InvalidInstances().emplace_back(lhs, attr);
        }
    });

    return result;
}

Validator::FDValidations Validator::ProcessFirstLevel(LhsPair const& lhs_pair) const {
    auto vertex = lhs_pair.first;
    auto const lhs = lhs_pair.second;
    auto const rhs = vertex->GetFDs();
    size_t const rhs_count = rhs.count();

    size_t const lhs_attr = lhs.find_first();
    if (lhs_attr == BitSet::npos) {
        return {};
    }

    FDValidations result;
    result.SetCountIntersections(rhs_count);
    result.SetCountValidations(rhs_count);

    util::ForEachIndex(rhs, [&](size_t attr) {
        for (auto const& cluster : plis_->at(lhs_attr)->GetIndex()) {
            size_t const cluster_id = (*compressed_records_)[cluster[0]][attr];
            if (algos::hy::PLIUtil::IsSingletonCluster(cluster_id) ||
                std::any_of(cluster.cbegin(), cluster.cend(), [this, attr, cluster_id](int id) {
                    return (*compressed_records_)[id][attr] != cluster_id;
                })) {
                vertex->RemoveFd(attr);
                result.InvalidInstances().emplace_back(lhs, attr);
                break;
            }
        }
    });
    return result;
}

Validator::FDValidations Validator::ProcessHigherLevel(LhsPair const& lhs_pair) {
    auto vertex = lhs_pair.first;
    auto lhs = lhs_pair.second;
    auto rhs = vertex->GetFDs();
    size_t const rhs_count = rhs.count();

    if (rhs_count == 0) {
        return {};
    }

    FDValidations result;
    result.SetCountValidations(rhs_count);
    result.SetCountIntersections(1);

    size_t const first_attr = lhs.find_first();

    lhs.reset(first_attr);
    BitSet const valid_rhss = Refine(result.ComparisonSuggestions(), *plis_, *compressed_records_,
                                     lhs, rhs, first_attr);
    lhs.set(first_attr);

    rhs &= ~valid_rhss;
    vertex->SetFds(valid_rhss);

    util::ForEachIndex(rhs, [&lhs, &result](size_t attr) {
        result.InvalidInstances().emplace_back(lhs, attr);
    });

    return result;
}

Validator::FDValidations Validator::GetValidations(LhsPair const& lhsPair) {
    if (GetLevelNum() == 0) {
        return ProcessZeroLevel(lhsPair);
    }

    if (GetLevelNum() == 1) {
        return ProcessFirstLevel(lhsPair);
    }

    return ProcessHigherLevel(lhsPair);
}

Validator::FDValidations Validator::ValidateAndExtendSeq(std::vector<LhsPair> const& vertices) {
    FDValidations result;
    for (auto const& vertex : vertices) {
        result.Add(GetValidations(vertex));
    }

    return result;
}

Validator::FDValidations Validator::ValidateAndExtendPar(std::vector<LhsPair> const& vertices) {
    FDValidations result;
    boost::asio::thread_pool pool(threads_num_);
    std::vector<std::future<FDValidations>> validation_futures;
    validation_futures.reserve(vertices.size());

    for (auto const& vertex : vertices) {
        std::packaged_task<FDValidations()> task(
                [this, &vertex]() { return GetValidations(vertex); });
        validation_futures.push_back(task.get_future());
        boost::asio::post(pool, std::move(task));
    }

    pool.join();

    for (auto&& future : validation_futures) {
        assert(future.valid());
        result.Add(future.get());
    }

    return result;
}

algos::hy::IdPairs Validator::ValidateAndExtendCandidates() {
    size_t const num_attributes = plis_->size();

    std::vector<LhsPair> cur_level_vertices;
    if (current_level_number_ != 0) {
        cur_level_vertices = fds_->GetLevel(current_level_number_);
    } else {
        cur_level_vertices.emplace_back(fds_->GetRootPtr(), BitSet(num_attributes));
    }

    size_t previous_num_invalid_fds = 0;
    algos::hy::IdPairs comparison_suggestions;
    while (!cur_level_vertices.empty()) {
        FDValidations result;
        if (threads_num_ > 1) {
            result = ValidateAndExtendPar(cur_level_vertices);
        } else {
            result = ValidateAndExtendSeq(cur_level_vertices);
        }

        comparison_suggestions.insert(comparison_suggestions.end(),
                                      result.ComparisonSuggestions().begin(),
                                      result.ComparisonSuggestions().end());
        if (current_level_number_ >= fds_->GetNumAttributes()) {
            break;
        }

        std::vector<LhsPair> next_level =
                algos::hy::CollectCurrentChildren(cur_level_vertices, num_attributes);
        size_t candidates = AddExtendedCandidatesFromInvalid(
                next_level, *fds_, result.InvalidInstances(), num_attributes, max_non_fds_);
        algos::hy::LogLevel(cur_level_vertices, result, candidates, current_level_number_, "FD");

        size_t const num_invalid_fds = result.InvalidInstances().size();
        size_t const num_valid_fds = result.CountValidations() - num_invalid_fds;
        cur_level_vertices = std::move(next_level);
        ++current_level_number_;

        if ((num_valid_fds == 0 || static_cast<double>(num_invalid_fds) / num_valid_fds >
                                           hyfd::HyFDConfig::kEfficiencyThreshold) &&
            previous_num_invalid_fds < num_invalid_fds) {
            return comparison_suggestions;
        }
        previous_num_invalid_fds = num_invalid_fds;
    }

    return {};
}

}  // namespace algos::cfdfinder
