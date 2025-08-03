#include "algorithms/md/md_verifier/md_verifier.h"

#include <algorithm>
#include <numeric>

#include "algorithms/md/hymd/utility/index_range.h"
#include "algorithms/md/lhs_column_similarity_classifier.h"
#include "algorithms/md/md_verifier/cmptr.h"
#include "algorithms/md/md_verifier/validation/validation.h"
#include "config/equal_nulls/option.h"
#include "config/exceptions.h"
#include "config/indices/option.h"
#include "config/indices/type.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "config/thread_number/option.h"
#include "util/timed_invoke.h"
#include "util/worker_thread_pool.h"

namespace {
struct PoolHolder {
private:
    std::optional<util::WorkerThreadPool> pool_holder_;
    util::WorkerThreadPool* pool_ptr_;

public:
    PoolHolder() : pool_holder_(), pool_ptr_(nullptr) {}

    PoolHolder(config::ThreadNumType threads) : pool_holder_(threads), pool_ptr_(&*pool_holder_) {}

    util::WorkerThreadPool* GetPtr() noexcept {
        return pool_ptr_;
    }
};

auto CreateSchema(config::InputTable const& table) {
    auto schema = std::make_shared<RelationalSchema>(table->GetRelationName());
    std::size_t const cols = table->GetNumberOfColumns();
    for (model::Index i : algos::hymd::utility::IndexRange(cols)) {
        schema->AppendColumn(table->GetColumnName(i));
    }
    return schema;
}
}  // namespace

namespace algos::md {
MDVerifier::MDVerifier() : Algorithm({}) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kLeftTable, kRightTable});
}

void MDVerifier::ResetState() {
    md_holds_ = true;
}

void MDVerifier::LoadDataInternal() {
    left_schema_ = CreateSchema(left_table_);
    right_schema_ = right_table_ ? CreateSchema(right_table_) : left_schema_;
}

void MDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto not_null = [](config::InputTable const& table) {
        if (table == nullptr) throw config::ConfigurationError("Left table may not be null.");
    };

    auto lhs_check = [this](std::vector<ColumnSimilarityClassifier> const& classifiers) {
        for (ColumnSimilarityClassifier const& classifier : classifiers) {
            if (classifier.GetDecisionBoundary() < 0 || classifier.GetDecisionBoundary() > 1) {
                throw config::ConfigurationError(
                        "Decision boundary " + std::to_string(classifier.GetDecisionBoundary()) +
                        "provided, but only decision boundaries in [0, 1] are correct.");
            }
            classifier.GetColumnMatch()->SetColumns(*left_schema_, *right_schema_);
        }
    };

    auto rhs_check = [this](ColumnSimilarityClassifier const& classifier) {
        if (classifier.GetDecisionBoundary() < 0 || classifier.GetDecisionBoundary() > 1) {
            throw config::ConfigurationError(
                    "Decision boundary " + std::to_string(classifier.GetDecisionBoundary()) +
                    "provided, but only decision boundaries in [0, 1] are correct.");
        }
        classifier.GetColumnMatch()->SetColumns(*left_schema_, *right_schema_);
    };

    RegisterOption(Option{&right_table_, kRightTable, kDRightTable, config::InputTable{nullptr}});
    RegisterOption(Option{&left_table_, kLeftTable, kDLeftTable}
                           .SetValueCheck(std::move(not_null))
                           .SetConditionalOpts({{{}, {kRightTable}}}));
    RegisterOption(Option{&lhs_, kMDLHS, kDMDLHS}.SetValueCheck(std::move(lhs_check)));
    RegisterOption(Option{&rhs_, kMDRHS, kDMDRHS}.SetValueCheck(std::move(rhs_check)));
    RegisterOption(config::kThreadNumberOpt(&threads_));
}

void MDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kRightTable, kLeftTable, kMDLHS, kMDRHS, kThreads});
}

unsigned long long MDVerifier::ExecuteInternal() {
    return util::TimedInvoke(&MDVerifier::VerifyMD, this);
}

model::MD MDVerifier::BuildMD(std::vector<ColumnSimilarityClassifier> const& lhs,
                              ColumnSimilarityClassifier const& rhs) {
    auto column_matches = std::make_shared<std::vector<model::md::ColumnMatch>>();
    auto get_column_match = [](ColumnSimilarityClassifier const& classifier) {
        auto [left_col_index, right_col_index] = classifier.GetColumnMatch()->GetIndices();
        std::string name = classifier.GetColumnMatch()->GetName();
        return model::md::ColumnMatch(left_col_index, right_col_index, std::move(name));
    };
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(*column_matches), get_column_match);
    column_matches->emplace_back(get_column_match(rhs));
    
    std::vector<model::md::LhsColumnSimilarityClassifier> lhs_transformed;
    model::Index column_match_current_index = 0;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(lhs_transformed),
                   [&column_match_current_index](ColumnSimilarityClassifier const& classifier) {
                       return model::md::LhsColumnSimilarityClassifier(
                               std::nullopt, column_match_current_index++,
                               classifier.GetDecisionBoundary());
                   });
    model::md::ColumnSimilarityClassifier rhs_transformed = model::md::ColumnSimilarityClassifier(
            column_match_current_index, rhs.GetDecisionBoundary());

    return model::MD(left_schema_, right_schema_, std::move(column_matches),
                     std::move(lhs_transformed), std::move(rhs_transformed));
}

MDValidationCalculator MDVerifier::CreateValidator() const {
    std::vector<CMPtr> column_matches;
    column_matches.reserve(lhs_.size() + 1);

    std::transform(lhs_.begin(), lhs_.end(), std::back_inserter(column_matches),
                   [](ColumnSimilarityClassifier const& classifier) {
                       return classifier.GetColumnMatch();
                   });
    column_matches.emplace_back(rhs_.GetColumnMatch());

    std::vector<model::md::ColumnSimilarityClassifier> lhs_column_similarity_classifiers;
    lhs_column_similarity_classifiers.reserve(lhs_.size());

    model::Index column_match_current_index = 0;
    std::transform(lhs_.begin(), lhs_.end(), std::back_inserter(lhs_column_similarity_classifiers),
                   [&column_match_current_index](ColumnSimilarityClassifier const& classifier) {
                       return model::md::ColumnSimilarityClassifier(
                               column_match_current_index++, classifier.GetDecisionBoundary());
                   });
    model::md::ColumnSimilarityClassifier rhs_column_similarity_classifier(
            column_match_current_index, rhs_.GetDecisionBoundary());

    MDValidationCalculator validator(left_table_, right_table_, std::move(column_matches),
                                     std::move(lhs_column_similarity_classifiers),
                                     std::move(rhs_column_similarity_classifier));

    return validator;
}

void MDVerifier::VerifyMD() {
    input_md_ = std::make_unique<model::MD>(BuildMD(lhs_, rhs_));

    auto pool_holder = threads_ > 1 ? PoolHolder(threads_) : PoolHolder();

    MDValidationCalculator validator = CreateValidator();

    validator.Validate(pool_holder.GetPtr());

    md_holds_ = validator.Holds();
    true_rhs_decision_boundary_ = validator.GetTrueRhsDecisionBoundary();

    md_suggestions_.push_back(BuildMD(
            lhs_, ColumnSimilarityClassifier(rhs_.GetColumnMatch(), true_rhs_decision_boundary_)));

    highlights_ = MDHighlights::CreateFrom(input_md_->GetDescription().rhs,
                                           validator.GetViolatingRecordsPairs(),
                                           validator.GetRhsPairsToSimilarityMapping());
}

}  // namespace algos::md
