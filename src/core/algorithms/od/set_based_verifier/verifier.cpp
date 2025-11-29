#include "core/algorithms/od/set_based_verifier/verifier.h"

#include <stdexcept>

#include "core/algorithms/od/fastod/model/canonical_od.h"
#include "core/config/column_index/option.h"
#include "core/config/indices/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/logger.h"
#include "core/util/range_to_string.h"
#include "core/util/timed_invoke.h"

namespace algos::od {

SetBasedAodVerifier::SetBasedAodVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void SetBasedAodVerifier::RegisterOptions() {
    using namespace config::names;
    using namespace config::descriptions;
    using config::ColumnIndexOption;
    using config::IndicesOption;
    using config::Option;

    auto get_cols_num = [this]() { return data_.GetColumnCount(); };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(
            IndicesOption{kOcContext, kDOcContext, true}(&oc_.context, get_cols_num)
                    .SetConditionalOpts({{nullptr, {kOcLeftIndex, kOcRightIndex, kOcLeftOrdering}}})
                    .SetIsRequiredFunc([this]() { return !OptionIsSet(kOFDContext); }));
    RegisterOption(config::ColumnIndexOption{kOcLeftIndex, kDOcLeftIndex}(&oc_.left, get_cols_num));
    RegisterOption(ColumnIndexOption{kOcRightIndex, kDOcRightIndex}(&oc_.right, get_cols_num));
    RegisterOption(Option<Ordering>{&oc_.left_ordering, kOcLeftOrdering, kDODLeftOrdering,
                                    Ordering::ascending});
    RegisterOption(IndicesOption{kOFDContext, kDOFDContext, true}(&ofd_.context, get_cols_num)
                           .SetConditionalOpts({{nullptr, {kOFDRightIndex}}})
                           .SetIsRequiredFunc([this]() { return !OptionIsSet(kOcContext); }));
    RegisterOption(ColumnIndexOption{kOFDRightIndex, kDOFDRightIndex}(&ofd_.right, get_cols_num));
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
    oc_ = OC{};
    ofd_ = OFD{};
    MakeOptionsAvailable({kOcContext, kOFDContext});
}

unsigned long long SetBasedAodVerifier::ExecuteInternal() {
    unsigned long long const elapsed_milliseconds =
            util::TimedInvoke(&SetBasedAodVerifier::Verify, this);

    LOG_DEBUG("AOD holds with error {}", GetError());
    LOG_DEBUG("Removal set: {}", util::RangeToString(removal_set_));

    return elapsed_milliseconds;
}

template <typename OD>
void SetBasedAodVerifier::CalculateRemovalSetForOD(OD const& od) {
    LOG_DEBUG("Processing {}: {}", OD::kName, od.ToString());
    RemovalSetAsVec removal_set = od.CalculateRemovalSet(data_, partition_cache_);
    LOG_DEBUG("{} removal set: {}", OD::kName, util::RangeToString(removal_set));
    removal_set_.insert(removal_set.begin(), removal_set.end());
}

template <od::Ordering Ordering>
void SetBasedAodVerifier::CalculateRemovalSetForOC() {
    fastod::AttributeSet oc_context =
            fastod::CreateAttributeSet(oc_.context, data_.GetColumnCount());
    fastod::CanonicalOD<Ordering> oc(oc_context, oc_.left, oc_.right);
    CalculateRemovalSetForOD(oc);
}

void SetBasedAodVerifier::CalculateRemovalSetForOFD() {
    fastod::AttributeSet ofd_context =
            fastod::CreateAttributeSet(ofd_.context, data_.GetColumnCount());
    fastod::SimpleCanonicalOD od(ofd_context, ofd_.right);
    CalculateRemovalSetForOD(od);
}

void SetBasedAodVerifier::Verify() {
    if (data_.GetTupleCount() == 0) {
        throw std::runtime_error("Input table is empty");
    }

    if (OptionIsSet(config::names::kOcContext)) {
        switch (oc_.left_ordering) {
            case od::Ordering::ascending:
                CalculateRemovalSetForOC<od::Ordering::ascending>();
                break;
            case od::Ordering::descending:
                CalculateRemovalSetForOC<od::Ordering::descending>();
                break;
            default:
                throw std::invalid_argument(std::string{"Unknown ordering: "} +
                                            oc_.left_ordering._to_string());
        }
    }

    if (OptionIsSet(config::names::kOFDContext)) {
        CalculateRemovalSetForOFD();
    }

    error_ = static_cast<Error>(removal_set_.size()) / data_.GetTupleCount();
    assert(removal_set_.size() < data_.GetTupleCount());
}

}  // namespace algos::od
