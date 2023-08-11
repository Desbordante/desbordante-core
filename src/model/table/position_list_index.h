//
// Created by Ilya Vologin
// https://github.com/cupertank
//

#pragma once
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

#include "model/table/column.h"

class ColumnLayoutRelationData;

namespace model {

class PositionListIndex {
public:
    /* Vector of tuple indices */
    using Cluster = std::vector<int>;

private:
    std::deque<Cluster> index_;
    Cluster null_cluster_;
    unsigned int size_;
    double entropy_;
    double inverted_entropy_;
    double gini_impurity_;
    unsigned long long nep_;
    unsigned int relation_size_;
    unsigned int original_relation_size_;
    std::shared_ptr<const std::vector<int>> probing_table_cache_;
    unsigned int freq_ = 0;

    static unsigned long long CalculateNep(unsigned int num_elements) {
        return static_cast<unsigned long long>(num_elements) * (num_elements - 1) / 2;
    }
    static void SortClusters(std::deque<Cluster>& clusters);
    static bool TakeProbe(int position, ColumnLayoutRelationData& relation_data,
                          Vertical const& probing_columns, std::vector<int>& probe);

public:
    static int intersection_count_;
    static unsigned long long micros_;
    static const int singleton_value_id_;

    PositionListIndex(std::deque<Cluster> index, Cluster null_cluster, unsigned int size,
                      double entropy, unsigned long long nep, unsigned int relation_size,
                      unsigned int original_relation_size, double inverted_entropy = 0,
                      double gini_impurity = 0);
    static std::unique_ptr<PositionListIndex> CreateFor(std::vector<int>& data,
                                                        bool is_null_eq_null);

    static std::unordered_map<int, unsigned> CreateFrequencies(
            Cluster const& cluster, std::vector<int> const& probing_table);

    // если PT закеширована, выдаёт её, иначе предварительно вычисляет её -- тяжёлая операция
    std::shared_ptr<const std::vector<int>> CalculateAndGetProbingTable() const;
    // выдаёт закешированную PT, либо nullptr, если она не закеширована
    std::vector<int> const* GetCachedProbingTable() const {
        return probing_table_cache_.get();
    };
    // кеширует PT
    void ForceCacheProbingTable() {
        probing_table_cache_ = CalculateAndGetProbingTable();
    };
    // Такая структура с кешированием ProbingTable нужна, потому что к PT одиночных колонок
    // происходят частые обращения, чтобы узнать какую-то одну конкретную позицию, тогда как PT
    // наборов колонок обычно используются, чтобы один раз пересечь две партиции, и больше к ним не
    // возвращаться

    // std::shared_ptr<const std::vector<int>> GetProbingTable(bool isCaching);

    std::deque<Cluster> const& GetIndex() const noexcept {
        return index_;
    };
    /* If you use this method and change index in any way, all other methods will become invalid */
    std::deque<Cluster>& GetIndex() noexcept {
        return index_;
    }
    double GetNep() const {
        return (double)nep_;
    }
    unsigned long long GetNepAsLong() const {
        return nep_;
    }
    unsigned int GetNumNonSingletonCluster() const {
        return index_.size();
    }
    unsigned int GetNumCluster() const {
        return index_.size() + original_relation_size_ - size_;
    }
    unsigned int GetFreq() const {
        return freq_;
    }
    unsigned int GetSize() const {
        return size_;
    }
    unsigned int getRelationSize() const {
        return relation_size_;
    }
    double GetEntropy() const {
        return entropy_;
    }
    double GetInvertedEntropy() const {
        return inverted_entropy_;
    }
    double GetGiniImpurity() const {
        return gini_impurity_;
    }
    double GetMaximumNip() const {
        return CalculateNep(relation_size_);
    }
    double GetNip() const {
        return GetMaximumNip() - GetNepAsLong();
    }

    bool AllValuesAreUnique() const noexcept {
        return GetNumNonSingletonCluster() == 0;
    }

    bool IsConstant() const {
        return relation_size_ <= 1 || (GetNumNonSingletonCluster() == 1 && size_ == relation_size_);
    }

    void IncFreq() {
        freq_++;
    }

    std::unique_ptr<PositionListIndex> Intersect(PositionListIndex const* that) const;
    std::unique_ptr<PositionListIndex> Probe(
            std::shared_ptr<const std::vector<int>> probing_table) const;
    std::unique_ptr<PositionListIndex> ProbeAll(Vertical const& probing_columns,
                                                ColumnLayoutRelationData& relation_data);
    std::string ToString() const;
};

using PLI = PositionListIndex;

}  // namespace model
