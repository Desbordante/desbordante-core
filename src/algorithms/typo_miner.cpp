#include "algorithms/typo_miner.h"

#include "algorithms/options/descriptions.h"
#include "algorithms/options/names.h"

namespace algos {

decltype(TypoMiner::RadiusOpt) TypoMiner::RadiusOpt{
        {config::names::kRadius, config::descriptions::kDRadius}, -1, [](auto value) {
            if (!(value == -1 || value >= 0)) {
                throw std::invalid_argument(
                        "Radius should be greater or equal to zero or equal to -1");
            }
        }};

decltype(TypoMiner::RatioOpt) TypoMiner::RatioOpt{
        {config::names::kRatio, config::descriptions::kDRatio}, {}, [](auto value) {
            if (!(value >= 0 && value <= 1)) {
                throw std::invalid_argument("Ratio should be between 0 and 1");
            }
        }};

TypoMiner::TypoMiner(PrimitiveType precise, PrimitiveType approx)
        : TypoMiner(CreatePrimitiveInstance<FDAlgorithm>(precise),
                    CreatePrimitiveInstance<FDAlgorithm>(approx)) {}

TypoMiner::TypoMiner(std::unique_ptr<FDAlgorithm> precise_algo,
                     std::unique_ptr<FDAlgorithm> approx_algo)
        : CsvPrimitive(std::vector<std::string_view>{
              /* "Precise fd algorithm execution", "Approximate fd algoritm execution",
               * "Extracting fds with non-zero error" */}),
          precise_algo_(std::move(precise_algo)),
          approx_algo_(std::move(approx_algo)) {
    RegisterOptions();
    MakeOptionsAvailable(config::GetOptionNames(config::EqualNullsOpt));
}

void TypoMiner::RegisterOptions() {
    RegisterOption(RadiusOpt.GetOption(&radius_));
    RegisterOption(RatioOpt.GetOption(&ratio_).OverrideDefaultFunction([this]() {
        return (relation_->GetNumRows() <= 1) ? 1 : (2.0 / relation_->GetNumRows());
    }));
    RegisterOption(config::EqualNullsOpt.GetOption(&is_null_equal_null_));
}

void TypoMiner::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable(config::GetOptionNames(RadiusOpt, RatioOpt));
}

void TypoMiner::ResetState() {
    approx_fds_.clear();
}

bool TypoMiner::HandleUnknownOption(std::string_view const& option_name,
                                    std::optional<boost::any> const& value) {
    using config::ErrorType;
    auto const& error_opt_type = config::ErrorOpt;
    if (option_name == error_opt_type.GetName()) {
        ErrorType val;
        error_opt_type.GetOption(&val)
                .OverrideDefaultValue({})
                .SetInstanceCheck([](auto value) {
                    if (value == 0.0)
                        throw std::invalid_argument("Typo mining with error 0 is meaningless");
                }).Set(value);
        if (TrySetOption(option_name, boost::any{ErrorType{0.0}}, boost::any{val}) == 0)
            return false;
        return true;
    }
    return static_cast<bool>(TrySetOption(option_name, value, value));
}

int TypoMiner::TrySetOption(std::string_view const& option_name,
                            std::optional<boost::any> const& value_precise,
                            std::optional<boost::any> const& value_approx) {
    int successes{};
    try {
        precise_algo_->SetOption(option_name, value_precise);
        ++successes;
    } catch (std::invalid_argument&) {}
    try {
        approx_algo_->SetOption(option_name, value_approx);
        ++successes;
    } catch (std::invalid_argument&) {}
    return successes;
}

void TypoMiner::AddSpecificNeededOptions(std::unordered_set<std::string_view>& previous_options) const {
    auto precise_options = precise_algo_->GetNeededOptions();
    auto approx_options = approx_algo_->GetNeededOptions();
    if (!FitCompleted()) {
        // blocked by TypoMiner's is_null_equal_null option
        precise_options.erase(config::names::kEqualNulls);
        approx_options.erase(config::names::kEqualNulls);
    }
    previous_options.insert(precise_options.begin(), precise_options.end());
    previous_options.insert(approx_options.begin(), approx_options.end());
}

void TypoMiner::FitInternal(model::IDatasetStream& data_stream) {
    relation_ = ColumnLayoutRelationData::CreateFrom(data_stream, is_null_equal_null_);
    data_stream.Reset();
    typed_relation_ =
            model::ColumnLayoutTypedRelationData::CreateFrom(data_stream, is_null_equal_null_);
    data_stream.Reset();
    auto precise_pli = dynamic_cast<PliBasedFDAlgorithm*>(precise_algo_.get());
    auto approx_pli = dynamic_cast<PliBasedFDAlgorithm*>(approx_algo_.get());
    std::optional<boost::any> null_opt_any{is_null_equal_null_};
    TrySetOption(config::EqualNullsOpt.GetName(), null_opt_any, null_opt_any);
    if (!precise_algo_->FitCompleted()) {
        if (precise_pli != nullptr)
            precise_pli->Fit(relation_);
        else
            precise_algo_->Fit(data_stream);
    }
    if (!approx_algo_->FitCompleted()) {
        if (approx_pli != nullptr)
            approx_pli->Fit(relation_);
        else
            approx_algo_->Fit(data_stream);
    }
}

unsigned long long TypoMiner::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();

    precise_algo_->Execute();
    approx_algo_->Execute();

    std::list<FD>& precise_fds = precise_algo_->FdList();
    std::list<FD>& approx_fds = approx_algo_->FdList();

    precise_fds.sort(FDLess);
    approx_fds.sort(FDLess);

    std::set_difference(approx_fds.begin(), approx_fds.end(), precise_fds.begin(),
                        precise_fds.end(), std::back_inserter(approx_fds_), FDLess);

    auto const elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start_time);

    return elapsed_milliseconds.count();
}

auto TypoMiner::MakeTuplesByIndicesComparator(std::map<int, unsigned> const& frequency_map) {
    return [&frequency_map](int const l, int const r) {
        return frequency_map.at(l) < frequency_map.at(r);
    };
}

std::vector<util::PLI::Cluster> TypoMiner::FindClustersWithTypos(FD const& typos_fd,
                                                                 bool const sort_clusters) const {
    std::vector<util::PLI::Cluster> clusters;
    std::shared_ptr<util::PLI const> intersection_pli;
    std::vector<Column const*> const lhs_columns = typos_fd.GetLhs().GetColumns();
    std::vector<int> const& probing_table =
        relation_->GetColumnData(typos_fd.GetRhs().GetIndex()).GetProbingTable();
    auto const sort_cluster = [this, &typos_fd](util::PLI::Cluster& cluster) {
        std::map<int, unsigned> const frequency_map =
            CreateFrequencyMap(typos_fd.GetRhs(), cluster);
        std::stable_sort(cluster.begin(), cluster.end(),
                         MakeTuplesByIndicesComparator(frequency_map));
    };

    if (lhs_columns.empty()) {
        /* If an approximate fd []->rhs holds then it is implied that rhs contains
         * only equal values with some deviations, which are considered errors,
         * so there is only one 'cluster with typos' containing all rows */
        util::PLI::Cluster cluster(relation_->GetNumRows());
        std::iota(cluster.begin(), cluster.end(), 0);
        if (sort_clusters) {
            sort_cluster(cluster);
        }
        return {cluster};
    }

    for (Column const* col : lhs_columns) {
        ColumnData const& col_data = relation_->GetColumnData(col->GetIndex());
        std::shared_ptr<util::PLI const> pli = col_data.GetPliOwnership();

        if (intersection_pli == nullptr) {
            intersection_pli = pli;
        } else {
            intersection_pli = intersection_pli->Intersect(pli.get());
        }
    }

    for (util::PLI::Cluster const& cluster : intersection_pli->GetIndex()) {
        int cluster_rhs_value = -1;

        /* Check if fd has wrong rhs values in this cluster */
        for (int const tuple_index : cluster) {
            int const probing_table_value = probing_table[tuple_index];

            if (cluster_rhs_value == -1) {
                cluster_rhs_value = probing_table_value;
            } else if (cluster_rhs_value != probing_table_value) {
                cluster_rhs_value = -1;
                break;
            }
        }

        if (cluster_rhs_value == -1 ||
            (ColumnData::IsValueSingleton(cluster_rhs_value) && cluster.size() != 1)) {
            /* Actually intersection_pli is owned only by this method most of the time
             * (when lhs_columns.size() != 1), so we can here move cluster when
             * lhs_columns.size() != 1. But that leads to more cumbersome and complex code.
             * So I decided to leave it as it is until we know for sure that this place causes
             * performance problems.
             */
            clusters.push_back(cluster);

            if (sort_clusters) {
                sort_cluster(clusters.back());
            }
        }
    }

    return clusters;
}

std::vector<TypoMiner::SquashedElement> TypoMiner::SquashCluster(
        FD const& squash_on, util::PLI::Cluster const& cluster) const {
    std::vector<SquashedElement> squashed;
    std::vector<int> const& probing_table =
            relation_->GetColumnData(squash_on.GetRhs().GetIndex()).GetProbingTable();

    if (cluster.empty()) {
        return squashed;
    }

    auto prev = cluster.begin();
    squashed.push_back({.tuple_index = *prev, .amount = 1});

    for (auto it = std::next(cluster.cbegin()); it != cluster.cend(); ++it) {
        if (probing_table[*it] != util::PLI::singleton_value_id_ &&
            probing_table[*it] == probing_table[*prev]) {
            squashed.back().amount++;
        } else {
            squashed.push_back({.tuple_index = *it, .amount = 1});
        }
        prev = it;
    }

    return squashed;
}

void TypoMiner::SortCluster(FD const& sort_on, util::PLI::Cluster& cluster) const {
    std::map<int, unsigned> const frequency_map = CreateFrequencyMap(sort_on.GetRhs(), cluster);

    std::stable_sort(cluster.begin(), cluster.end(), MakeTuplesByIndicesComparator(frequency_map));
}

void TypoMiner::RestoreLineOrder(util::PLI::Cluster& cluster) const {
    std::sort(cluster.begin(), cluster.end());
}

void TypoMiner::RestoreLineOrder(std::vector<TypoMiner::SquashedElement>& squashed_cluster) const {
    std::sort(squashed_cluster.begin(), squashed_cluster.end(),
              [](const TypoMiner::SquashedElement& lhs, const TypoMiner::SquashedElement& rhs) {
                  return lhs.tuple_index < rhs.tuple_index;
              });
}

std::vector<util::PLI::Cluster::value_type> TypoMiner::FindLinesWithTypos(
        FD const& typos_fd, util::PLI::Cluster const& cluster, double new_radius,
        double new_ratio) {
    SetRadius(new_radius);
    SetRatio(new_ratio);
    return FindLinesWithTypos(typos_fd, cluster);
}

std::vector<util::PLI::Cluster::value_type> TypoMiner::FindLinesWithTypos(
    FD const& typos_fd, util::PLI::Cluster const& cluster) const {
    Column const& col = typos_fd.GetRhs();
    model::TypedColumnData const& col_data = typed_relation_->GetColumnData(col.GetIndex());
    std::vector<int> const& probing_table =
        relation_->GetColumnData(col.GetIndex()).GetProbingTable();
    model::Type const& type = col_data.GetType();

    unsigned most_freq_index = GetMostFrequentValueIndex(col, cluster);
    int most_freq_value = probing_table[most_freq_index];

    if (ColumnData::IsValueSingleton(most_freq_value) || col_data.IsMixed() ||
        !type.IsMetrizable() || col_data.IsEmpty(most_freq_index) ||
        col_data.IsNullOrEmpty(most_freq_index)) {
        if (ratio_ == 1) {
            return cluster;
        }
        return {};
    }

    std::vector<util::PLI::Cluster::value_type> typos;
    unsigned long num_of_close_values = 0;
    std::vector<std::byte const*> const& data = col_data.GetData();

    for (util::PLI::Cluster::value_type tuple_index : cluster) {
        /* Temporary ignoring NULL or empty values. Maybe it should be decided by some parameter
         * if NULL (empty) value is close to any non-NULL (non-empty) or not (as in metric
         * dependecies).
         */
        if (most_freq_value == probing_table[tuple_index] || col_data.IsNullOrEmpty(tuple_index)) {
            continue;
        }
        if (radius_ == -1 || ValuesAreClose(data[most_freq_index], data[tuple_index], type)) {
            num_of_close_values++;
            typos.push_back(tuple_index);
        }
    }

    if (double(num_of_close_values) / col_data.GetNumRows() > ratio_) {
        return {};
    }

    return typos;
}

std::vector<TypoMiner::ClusterTyposPair> TypoMiner::FindClustersAndLinesWithTypos(
        const FD& typos_fd, const bool sort_clusters) const {
    std::vector<ClusterTyposPair> result;
    std::vector<util::PLI::Cluster> clusters = FindClustersWithTypos(typos_fd, sort_clusters);

    result.reserve(clusters.size());

    for (auto& cluster : clusters) {
        TyposVec typos = FindLinesWithTypos(typos_fd, cluster);
        if (!typos.empty()) {
            result.emplace_back(std::move(cluster), std::move(typos));
        }
    }

    return result;
}

std::unordered_map<int, unsigned> TypoMiner::CreateFrequencies(
    util::PLI::Cluster const& cluster, std::vector<int> const& probing_table) const {
    std::unordered_map<int, unsigned> frequencies;

    for (int const tuple_index : cluster) {
        int const probing_table_value = probing_table[tuple_index];

        if (probing_table_value != 0) {
            frequencies[probing_table_value]++;
        }
    }

    return frequencies;
}

unsigned TypoMiner::GetMostFrequentValueIndex(Column const& cluster_col,
                                              util::PLI::Cluster const& cluster) const {
    assert(!cluster.empty());
    std::vector<int> const& probing_table =
        relation_->GetColumnData(cluster_col.GetIndex()).GetProbingTable();
    std::unordered_map<int, unsigned> frequencies = CreateFrequencies(cluster, probing_table);

    unsigned most_frequent_index = cluster.size();
    unsigned largest_frequency = 0;
    for (int const tuple_index : cluster) {
        int const probing_table_value = probing_table[tuple_index];
        unsigned const frequency = (ColumnData::IsValueSingleton(probing_table_value))
                                   ? 1
                                   : frequencies.at(probing_table_value);
        if (frequency > largest_frequency) {
            largest_frequency = frequency;
            most_frequent_index = tuple_index;
        }
    }

    return most_frequent_index;
}

std::map<int, unsigned> TypoMiner::CreateFrequencyMap(Column const& cluster_col,
                                                      util::PLI::Cluster const& cluster) const {
    std::map<int, unsigned> frequency_map;
    std::vector<int> const& probing_table =
        relation_->GetColumnData(cluster_col.GetIndex()).GetProbingTable();
    std::unordered_map<int, unsigned> frequencies = CreateFrequencies(cluster, probing_table);

    for (int const tuple_index : cluster) {
        int const probing_table_value = probing_table[tuple_index];
        unsigned const value = (ColumnData::IsValueSingleton(probing_table_value))
                                   ? 1
                                   : frequencies.at(probing_table_value);
        frequency_map[tuple_index] = value;
    }

    return frequency_map;
}

bool TypoMiner::FDLess(FD const& l, FD const& r) {
    if (l.GetLhs() < r.GetLhs()) {
        return true;
    } else if (l.GetLhs() == r.GetLhs()) {
        return l.GetRhs() < r.GetRhs();
    }

    return false;
}

}  // namespace algos
