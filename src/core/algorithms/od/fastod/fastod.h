#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/od/fastod/model/attribute_pair.h"
#include "core/algorithms/od/fastod/model/attribute_set.h"
#include "core/algorithms/od/fastod/model/canonical_od.h"
#include "core/algorithms/od/fastod/storage/partition_cache.h"
#include "core/algorithms/od/fastod/util/timer.h"
#include "core/config/error/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/config/time_limit/type.h"

namespace algos {

class Fastod : public Algorithm {
private:
    using AscCanonicalOD = fastod::AscCanonicalOD;
    using DescCanonicalOD = fastod::DescCanonicalOD;
    using SimpleCanonicalOD = fastod::SimpleCanonicalOD;
    using AttributePair = fastod::AttributePair;
    using AttributeSet = fastod::AttributeSet;
    using PartitionCache = fastod::PartitionCache;
    using DataFrame = fastod::DataFrame;
    using Timer = fastod::Timer;

    config::TimeLimitSecondsType time_limit_seconds_ = 0u;
    bool is_complete_ = true;
    size_t level_ = 1;

    std::vector<AscCanonicalOD> result_asc_;
    std::vector<DescCanonicalOD> result_desc_;
    std::vector<SimpleCanonicalOD> result_simple_;

    std::unordered_set<AttributeSet> context_in_current_level_;
    std::unordered_map<AttributeSet, AttributeSet> cc_;
    std::unordered_map<AttributeSet, std::unordered_set<AttributePair>> cs_asc_;
    std::unordered_map<AttributeSet, std::unordered_set<AttributePair>> cs_desc_;

    Timer timer_;
    PartitionCache partition_cache_;

    AttributeSet schema_;
    DataFrame data_;
    config::InputTable input_table_;
    config::ErrorType error_;

    void MakeExecuteOptsAvailable() override;
    void LoadDataInternal() override;
    void ResetState() override;
    unsigned long long ExecuteInternal() final;

    void PrepareOptions();
    void RegisterOptions();
    void MakeLoadOptionsAvailable();

    bool IsTimeUp() const;

    void Initialize();
    void ComputeODs();
    void PruneLevels();
    void CalculateNextLevel();
    void Discover();

    void CCPut(AttributeSet const& key, AttributeSet attribute_set);
    AttributeSet const& CCGet(AttributeSet const& key);

    template <od::Ordering Ordering>
    [[nodiscard]] static consteval bool IsAscending() {
        return Ordering == +od::Ordering::ascending;
    }

    template <od::Ordering Ordering>
    void CSPut(AttributeSet const& key, AttributePair const& value) {
        if constexpr (IsAscending<Ordering>()) {
            cs_asc_[key].emplace(value);
        } else {
            cs_desc_[key].emplace(value);
        }
    }

    template <od::Ordering Ordering>
    void CSPut(AttributeSet const& key, AttributePair&& value) {
        if constexpr (IsAscending<Ordering>()) {
            cs_asc_[key].emplace(std::move(value));
        } else {
            cs_desc_[key].emplace(std::move(value));
        }
    }

    template <od::Ordering Ordering>
    std::unordered_set<AttributePair>& CSGet(AttributeSet const& key) {
        if constexpr (IsAscending<Ordering>()) {
            return cs_asc_[key];
        } else {
            return cs_desc_[key];
        }
    }

    template <od::Ordering Ordering>
    void AddToResult(fastod::CanonicalOD<Ordering>&& od) {
        if constexpr (IsAscending<Ordering>()) {
            result_asc_.emplace_back(std::move(od));
        } else {
            result_desc_.emplace_back(std::move(od));
        }
    }

    void AddToResult(SimpleCanonicalOD&& od) {
        result_simple_.emplace_back(std::move(od));
    }

    template <od::Ordering Ordering>
    void AddCandidates(AttributeSet const& context,
                       std::vector<AttributeSet> const& deleted_attrs) {
        if (level_ == 2) {
            for (model::ColumnIndex i = 0; i < data_.GetColumnCount(); i++) {
                for (model::ColumnIndex j = 0; j < data_.GetColumnCount(); j++) {
                    if (i == j) continue;
                    CSPut<Ordering>(fastod::CreateAttributeSet(
                                            std::initializer_list<model::ColumnIndex>{i, j},
                                            data_.GetColumnCount()),
                                    AttributePair(i, j));
                }
            }
        } else if (level_ > 2) {
            context.Iterate([this, &deleted_attrs, &context](model::ColumnIndex attr) {
                auto const& candidates = CSGet<Ordering>(deleted_attrs[attr]);

                for (AttributePair const& attribute_pair : candidates) {
                    AttributeSet const context_delete_ab = fastod::DeleteAttribute(
                            deleted_attrs[attribute_pair.left], attribute_pair.right);

                    bool add_context = true;

                    context_delete_ab.Iterate([this, &deleted_attrs, &attribute_pair,
                                               &add_context](model::ColumnIndex attr) {
                        std::unordered_set<AttributePair> const& cs =
                                CSGet<Ordering>(deleted_attrs[attr]);

                        if (cs.find(attribute_pair) == cs.end()) {
                            add_context = false;
                            return;
                        }
                    });

                    if (add_context) {
                        CSPut<Ordering>(context, attribute_pair);
                    }
                }
            });
        }
    }

    template <od::Ordering Ordering>
    void CalculateODs(AttributeSet const& context, std::vector<AttributeSet> const& deleted_attrs) {
        auto& cs_for_con = CSGet<Ordering>(context);

        for (auto it = cs_for_con.begin(); it != cs_for_con.end();) {
            model::ColumnIndex a = it->left;
            model::ColumnIndex b = it->right;

            if (ContainsAttribute(CCGet(deleted_attrs[b]), a) &&
                ContainsAttribute(CCGet(deleted_attrs[a]), b)) {
                fastod::CanonicalOD<Ordering> od(fastod::DeleteAttribute(deleted_attrs[a], b), a,
                                                 b);

                if (od.IsValid(data_, partition_cache_, error_)) {
                    AddToResult(std::move(od));
                    cs_for_con.erase(it++);
                } else {
                    ++it;
                }
            } else {
                cs_for_con.erase(it++);
            }
        }
    }

public:
    Fastod();

    void PrintStatistics() const;
    bool IsComplete() const;

    std::vector<AscCanonicalOD> const& GetAscendingDependencies() const;
    std::vector<DescCanonicalOD> const& GetDescendingDependencies() const;
    std::vector<SimpleCanonicalOD> const& GetSimpleDependencies() const;
};

}  // namespace algos
