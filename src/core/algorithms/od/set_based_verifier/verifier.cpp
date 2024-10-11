#include "verifier.h"

#include <chrono>
#include <stdexcept>

#include <easylogging++.h>

#include "algorithms/od/fastod/model/canonical_od.h"
#include "config/column_index/option.h"
#include "config/indices/option.h"
#include "config/names_and_descriptions.h"
#include "config/tabular_data/input_table/option.h"
#include "util/range_to_string.h"
#include "util/timed_invoke.h"

namespace algos::od {

SetBasedAodVerifier::SetBasedAodVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void SetBasedAodVerifier::RegisterOptions() {
    using namespace config::names;
    using namespace config::descriptions;
    using config::Option;

    auto get_cols_num = [this]() { return data_.GetColumnCount(); };
    auto oc_before_set = [this]() {
        if (!oc_.has_value()) {
            oc_ = OC{};
        }
    };
    auto ofd_before_set = [this]() {
        if (!ofd_.has_value()) {
            ofd_ = OFD{};
        }
    };
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(
            config::IndicesOption{kOcContext, kDOcContext, true}(&oc_->context, get_cols_num)
                    .SetBeforeSetCallback(oc_before_set)
                    .SetConditionalOpts({{nullptr, {kOcLeftIndex, kOcRightIndex, kOcLeftOrdering}}})
                    .SetIsRequiredFunc([this]() { return !ofd_.has_value(); }));
    RegisterOption(
            config::ColumnIndexOption{kOcLeftIndex, kDOcLeftIndex}(&oc_->left, get_cols_num));
    RegisterOption(
            config::ColumnIndexOption{kOcRightIndex, kDOcRightIndex}(&oc_->right, get_cols_num));
    RegisterOption(Option<Ordering>{&oc_->left_ordering, kOcLeftOrdering, kDODLeftOrdering,
                                    Ordering::ascending});
    RegisterOption(
            config::IndicesOption{kOFDContext, kDOFDContext, true}(&ofd_->context, get_cols_num)
                    .SetBeforeSetCallback(ofd_before_set)
                    .SetConditionalOpts({{nullptr, {kOFDRightIndex}}})
                    .SetIsRequiredFunc([this]() { return !oc_.has_value(); }));
    RegisterOption(
            config::ColumnIndexOption{kOFDRightIndex, kDOFDRightIndex}(&ofd_->right, get_cols_num));
}

void SetBasedAodVerifier::ResetState() {
    error_ = 0;
    removal_set_.clear();
}

void SetBasedAodVerifier::LoadDataInternal() {
    data_ = fastod::DataFrame::FromInputTable(input_table_);
}

void SetBasedAodVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    oc_ = std::nullopt;
    ofd_ = std::nullopt;
    MakeOptionsAvailable({kOcContext, kOFDContext});
}

unsigned long long SetBasedAodVerifier::ExecuteInternal() {
    unsigned long long const elapsed_milliseconds =
            util::TimedInvoke(&SetBasedAodVerifier::Verify, this);

    LOG(DEBUG) << "AOD holds with error " << GetError();
    LOG(DEBUG) << "Removal set: " << util::RangeToString(removal_set_);

    return elapsed_milliseconds;
}

template <od::Ordering Ordering>
void SetBasedAodVerifier::CalculateRemovalSetForOC() {
    fastod::AttributeSet oc_context =
            fastod::CreateAttributeSet(oc_->context, data_.GetColumnCount());
    fastod::CanonicalOD<Ordering> oc(oc_context, oc_->left, oc_->right);
    LOG(DEBUG) << "Processing OC: " << oc.ToString();
    RemovalSetAsVec oc_removal_set = oc.CalculateRemovalSet(data_, partition_cache_);
    LOG(DEBUG) << "OC removal set: " << util::RangeToString(oc_removal_set);
    removal_set_.insert(oc_removal_set.begin(), oc_removal_set.end());
}

void SetBasedAodVerifier::CalculateRemovalSetForOFD() {
    fastod::AttributeSet ofd_context =
            fastod::CreateAttributeSet(ofd_->context, data_.GetColumnCount());
    fastod::SimpleCanonicalOD od(ofd_context, ofd_->right);
    LOG(DEBUG) << "Processing OFD: " << od.ToString();
    RemovalSetAsVec ofd_removal_set = od.CalculateRemovalSet(data_, partition_cache_);
    LOG(DEBUG) << "OFD removal set: " << util::RangeToString(ofd_removal_set);
    removal_set_.insert(ofd_removal_set.begin(), ofd_removal_set.end());
}

void SetBasedAodVerifier::Verify() {
    if (data_.GetTupleCount() == 0) {
        throw std::runtime_error("Input table is empty");
    }

    if (oc_.has_value()) {
        switch (oc_->left_ordering) {
            case od::Ordering::ascending:
                CalculateRemovalSetForOC<od::Ordering::ascending>();
                break;
            case od::Ordering::descending:
                CalculateRemovalSetForOC<od::Ordering::descending>();
                break;
            default:
                throw std::invalid_argument(std::string{"Unknown ordering: "} +
                                            oc_->left_ordering._to_string());
        }
    }

    if (ofd_.has_value()) {
        CalculateRemovalSetForOFD();
    }

    error_ = static_cast<Error>(removal_set_.size()) / data_.GetTupleCount();
    assert(removal_set_.size() < data_.GetTupleCount());
}

}  // namespace algos::od
