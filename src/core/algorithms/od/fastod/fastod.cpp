#include "fastod.h"

#include <memory>

#include <boost/unordered/unordered_map.hpp>
#include <easylogging++.h>

#include "config/tabular_data/input_table/option.h"
#include "config/time_limit/option.h"
#include "error/option.h"
#include "util/timed_invoke.h"

namespace algos {

Fastod::Fastod() : Algorithm({}) {
    PrepareOptions();
}

bool Fastod::IsTimeUp() const {
    return time_limit_seconds_ > 0 && timer_.GetElapsedSeconds() >= time_limit_seconds_;
}

void Fastod::CCPut(AttributeSet const& key, AttributeSet attribute_set) {
    cc_[key] = std::move(attribute_set);
}

fastod::AttributeSet const& Fastod::CCGet(AttributeSet const& key) {
    return cc_[key];
}

void Fastod::PrepareOptions() {
    RegisterOptions();
    MakeLoadOptionsAvailable();
}

void Fastod::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kTimeLimitSecondsOpt(&time_limit_seconds_));
    RegisterOption(config::kErrorOpt(&error_));
}

void Fastod::MakeLoadOptionsAvailable() {
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void Fastod::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kTimeLimitSecondsOpt.GetName(), config::kErrorOpt.GetName()});
}

void Fastod::LoadDataInternal() {
    data_ = DataFrame::FromInputTable(input_table_);
}

void Fastod::ResetState() {
    is_complete_ = false;
    level_ = 1;

    result_asc_.clear();
    result_desc_.clear();
    result_simple_.clear();
    context_in_current_level_.clear();
    cc_.clear();
    cs_asc_.clear();
    cs_desc_.clear();

    timer_ = Timer();
    partition_cache_.Clear();
}

unsigned long long Fastod::ExecuteInternal() {
    size_t const elapsed_milliseconds = util::TimedInvoke(&Fastod::Discover, this);

    for (auto const& od : result_asc_) {
        LOG(DEBUG) << od.ToString();
    }

    for (auto const& od : result_desc_) {
        LOG(DEBUG) << od.ToString();
    }

    for (auto const& od : result_simple_) {
        LOG(DEBUG) << od.ToString();
    }

    return elapsed_milliseconds;
}

void Fastod::PrintStatistics() const {
    const size_t ocd_count = result_asc_.size() + result_desc_.size();
    const size_t fd_count = result_simple_.size();
    const size_t od_count = ocd_count + fd_count;

    LOG(DEBUG) << "RESULT: Time=" << timer_.GetElapsedSeconds() << ", "
               << "OD=" << od_count << ", "
               << "FD=" << fd_count << ", "
               << "OCD=" << ocd_count;
}

bool Fastod::IsComplete() const {
    return is_complete_;
}

std::vector<fastod::AscCanonicalOD> const& Fastod::GetAscendingDependencies() const {
    return result_asc_;
}

std::vector<fastod::DescCanonicalOD> const& Fastod::GetDescendingDependencies() const {
    return result_desc_;
}

std::vector<fastod::SimpleCanonicalOD> const& Fastod::GetSimpleDependencies() const {
    return result_simple_;
}

void Fastod::Initialize() {
    timer_.Start();

    schema_ = AttributeSet(data_.GetColumnCount(), (1 << data_.GetColumnCount()) - 1);

    AttributeSet empty_set(data_.GetColumnCount());
    CCPut(std::move(empty_set), schema_);

    for (model::ColumnIndex i = 0; i < data_.GetColumnCount(); ++i)
        context_in_current_level_.emplace(data_.GetColumnCount(), 1 << i);
}

void Fastod::ComputeODs() {
    Timer timer(true);
    std::vector<std::vector<AttributeSet>> deleted_attrs(context_in_current_level_.size());
    size_t context_ind = 0;

    for (AttributeSet const& context : context_in_current_level_) {
        auto& del_attrs = deleted_attrs[context_ind++];
        del_attrs.reserve(data_.GetColumnCount());

        for (model::ColumnIndex column = 0; column < data_.GetColumnCount(); ++column) {
            del_attrs.push_back(fastod::DeleteAttribute(context, column));
        }

        if (IsTimeUp()) {
            is_complete_ = false;
            return;
        }

        AttributeSet context_cc = schema_;

        context.Iterate([this, &context_cc, &del_attrs](model::ColumnIndex attr) {
            context_cc = fastod::Intersect(context_cc, CCGet(del_attrs[attr]));
        });

        CCPut(context, context_cc);

        AddCandidates<od::Ordering::descending>(context, del_attrs);
        AddCandidates<od::Ordering::ascending>(context, del_attrs);
    }

    size_t delete_index = 0;

    for (AttributeSet const& context : context_in_current_level_) {
        auto const& del_attrs = deleted_attrs[delete_index++];

        if (IsTimeUp()) {
            is_complete_ = false;
            return;
        }

        AttributeSet const& cc = CCGet(context);
        AttributeSet context_intersect_cc_context = fastod::Intersect(context, cc);

        context_intersect_cc_context.Iterate(
                [this, &context, &del_attrs, &cc](model::ColumnIndex attr) {
                    SimpleCanonicalOD od(del_attrs[attr], attr);

                    if (od.IsValid(data_, partition_cache_, error_)) {
                        AddToResult(std::move(od));
                        CCPut(context, fastod::DeleteAttribute(cc, attr));

                        const AttributeSet diff = fastod::Difference(schema_, context);

                        if (diff.Any()) {
                            CCPut(context, cc & (~diff));
                        }
                    }
                });

        CalculateODs<od::Ordering::descending>(context, del_attrs);
        CalculateODs<od::Ordering::ascending>(context, del_attrs);
    }
}

void Fastod::PruneLevels() {
    if (level_ == 1) {
        return;
    }

    for (auto attribute_set_it = context_in_current_level_.begin();
         attribute_set_it != context_in_current_level_.end();) {
        if (IsEmptySet(CCGet(*attribute_set_it)) &&
            CSGet<od::Ordering::ascending>(*attribute_set_it).empty() &&
            CSGet<od::Ordering::descending>(*attribute_set_it).empty()) {
            context_in_current_level_.erase(attribute_set_it++);
        } else {
            ++attribute_set_it;
        }
    }
}

void Fastod::CalculateNextLevel() {
    boost::unordered_map<AttributeSet, std::vector<size_t>> prefix_blocks;
    std::unordered_set<AttributeSet> context_next_level;

    for (AttributeSet const& attribute_set : context_in_current_level_) {
        attribute_set.Iterate([&prefix_blocks, &attribute_set](model::ColumnIndex attr) {
            prefix_blocks[fastod::DeleteAttribute(attribute_set, attr)].push_back(attr);
        });
    }

    for (auto const& [prefix, single_attributes] : prefix_blocks) {
        if (IsTimeUp()) {
            is_complete_ = false;
            return;
        }

        if (single_attributes.size() <= 1) {
            continue;
        }

        for (size_t i = 0; i < single_attributes.size(); ++i) {
            for (size_t j = i + 1; j < single_attributes.size(); ++j) {
                bool create_context = true;

                const AttributeSet candidate = fastod::AddAttribute(
                        fastod::AddAttribute(prefix, single_attributes[i]), single_attributes[j]);

                candidate.Iterate([this, &candidate, &create_context](model::ColumnIndex attr) {
                    if (context_in_current_level_.find(fastod::DeleteAttribute(candidate, attr)) ==
                        context_in_current_level_.end()) {
                        create_context = false;
                        return;
                    }
                });

                if (create_context) {
                    context_next_level.insert(candidate);
                }
            }
        }
    }

    context_in_current_level_ = std::move(context_next_level);
}

void Fastod::Discover() {
    Initialize();

    while (!context_in_current_level_.empty()) {
        ComputeODs();

        if (IsTimeUp()) {
            break;
        }

        PruneLevels();
        CalculateNextLevel();

        if (IsTimeUp()) {
            break;
        }

        level_++;
    }

    timer_.Stop();

    if (IsComplete()) {
        LOG(DEBUG) << "FastOD finished successfully";
    } else {
        LOG(DEBUG) << "FastOD finished with a time-out";
    }

    PrintStatistics();
}

}  // namespace algos
