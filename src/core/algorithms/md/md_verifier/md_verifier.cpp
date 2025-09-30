#include "algorithms/md/md_verifier/md_verifier.h"

#include <algorithm>
#include <optional>

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

    auto check_classifier = [this](ColumnSimilarityClassifier const& classifier) {
        double const decision_boundary = classifier.GetDecisionBoundary();
        if (decision_boundary < 0.0 || decision_boundary > 1.0) {
            throw config::ConfigurationError("Decision boundary" +
                                             std::to_string(decision_boundary) +
                                             "provided, but only decision boundaries in [0, 1] are "
                                             "correct.");
        }
        classifier.GetColumnMatch()->SetColumns(*left_schema_, *right_schema_);
    };

    auto lhs_check =
            [check = check_classifier](std::vector<ColumnSimilarityClassifier> const& classifiers) {
                std::ranges::for_each(classifiers, check);
            };

    RegisterOption(Option{&right_table_, kRightTable, kDRightTable, config::InputTable{nullptr}});
    RegisterOption(Option{&left_table_, kLeftTable, kDLeftTable}
                           .SetValueCheck(std::move(not_null))
                           .SetConditionalOpts({{{}, {kRightTable}}}));
    RegisterOption(Option{&lhs_, kMDLHS, kDMDLHS}.SetValueCheck(std::move(lhs_check)));
    RegisterOption(Option{&rhs_, kMDRHS, kDMDRHS}.SetValueCheck(std::move(check_classifier)));
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
    column_matches->reserve(lhs.size() + 1);
    auto get_column_match = [](ColumnSimilarityClassifier const& classifier) {
        auto [left_col_index, right_col_index] = classifier.GetColumnMatch()->GetIndices();
        std::string name = classifier.GetColumnMatch()->GetName();
        return model::md::ColumnMatch(left_col_index, right_col_index, std::move(name));
    };
    std::ranges::transform(lhs, std::back_inserter(*column_matches), get_column_match);
    column_matches->emplace_back(get_column_match(rhs));

    std::vector<model::md::LhsColumnSimilarityClassifier> lhs_transformed;
    lhs_transformed.reserve(lhs.size());
    model::Index column_match_current_index = 0;
    for (ColumnSimilarityClassifier const& classifier : lhs) {
        lhs_transformed.emplace_back(std::nullopt, column_match_current_index++,
                                     classifier.GetDecisionBoundary());
    }
    model::md::ColumnSimilarityClassifier rhs_transformed = model::md::ColumnSimilarityClassifier(
            column_match_current_index, rhs.GetDecisionBoundary());

    return model::MD(left_schema_, right_schema_, std::move(column_matches),
                     std::move(lhs_transformed), std::move(rhs_transformed));
}

MDValidationCalculator MDVerifier::CreateValidator() const {
    std::vector<CMPtr> column_matches;
    column_matches.reserve(lhs_.size() + 1);

    std::ranges::transform(lhs_, std::back_inserter(column_matches),
                           &ColumnSimilarityClassifier::GetColumnMatch);
    column_matches.emplace_back(rhs_.GetColumnMatch());

    std::vector<model::md::ColumnSimilarityClassifier> column_similarity_classifiers;
    column_similarity_classifiers.reserve(lhs_.size() + 1);

    model::Index column_match_current_index = 0;
    for (ColumnSimilarityClassifier const& classifier : lhs_) {
        column_similarity_classifiers.emplace_back(column_match_current_index++,
                                                   classifier.GetDecisionBoundary());
    }
    column_similarity_classifiers.emplace_back(column_match_current_index,
                                               rhs_.GetDecisionBoundary());

    MDValidationCalculator validator(left_table_, right_table_, std::move(column_matches),
                                     std::move(column_similarity_classifiers), highlights_);

    return validator;
}

void MDVerifier::VerifyMD() {
    input_md_ = BuildMD(lhs_, rhs_);
    highlights_ = std::make_shared<MDHighlights>(input_md_->GetDescription().rhs);

    std::optional<util::WorkerThreadPool> pool;
    if (threads_ > 1) {
        pool.emplace(threads_);
    }

    MDValidationCalculator validator = CreateValidator();

    validator.Validate(pool ? std::addressof(pool.value()) : nullptr);

    md_holds_ = validator.Holds();
    true_rhs_decision_boundary_ = validator.GetTrueRhsDecisionBoundary();

    md_suggestion_ = BuildMD(
            lhs_, ColumnSimilarityClassifier(rhs_.GetColumnMatch(), true_rhs_decision_boundary_));
}

}  // namespace algos::md
