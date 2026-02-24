#include "core/algorithms/fd/fd_verifier/fd_verifier.h"

#include <chrono>
#include <memory>
#include <stdexcept>

#include "core/config/equal_nulls/option.h"
#include "core/config/indices/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/normalize_indices.h"

namespace {
std::vector<model::Index> ConvertToIndexVector(
        std::vector<std::variant<std::string, model::Index>> const& variant_vector) {
    std::vector<model::Index> result;
    result.reserve(variant_vector.size());

    for (auto const& variant : variant_vector) {
        result.push_back(std::get<model::Index>(variant));
    }

    return result;
}
}  // namespace

namespace algos::fd_verifier {

FDVerifier::FDVerifier() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void FDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto not_empty = [](model::FdInput const& fd_input) {
        if (fd_input.rhs.empty()) {
            throw config::ConfigurationError("RHS is empty, there is nothing to check");
        }
        // TODO: fix this, they should be
        if (fd_input.lhs.empty()) {
            throw config::ConfigurationError("FDs with empty LHS are unsupported");
        }
    };
    auto normalize_index = [this](auto&& arg) -> model::Index {
        using T = std::decay_t<decltype(arg)>;

        std::vector<std::string> const& column_names = table_header_.column_names;
        if constexpr (std::is_same_v<T, std::string>) {
            auto it = std::find(column_names.begin(), column_names.end(), arg);

            if (it == column_names.end()) {
                throw config::ConfigurationError("No column named \"" + arg + "\"");
            }

            if (std::find(std::next(it), column_names.end(), arg) != column_names.end()) {
                throw config::ConfigurationError(
                        "Reference to " + arg +
                        "in column identifier list is ambiguous, use its index to disambigulate");
            }

            return std::distance(column_names.begin(), it);
        } else {
            static_assert(std::is_same_v<T, model::Index>);
            std::size_t const column_number = column_names.size();
            if (arg >= column_number) {
                throw config::ConfigurationError("Column index " + std::to_string(arg) +
                                                 "is out of bounds, only " +
                                                 std::to_string(column_number) + " exist!");
            }
            return arg;
        }
    };
    auto normalize_fd_input = [normalize_index](model::FdInput& fd_input) {
        for (std::variant<std::string, model::Index>& s : fd_input.lhs) {
            s = std::visit(normalize_index, s);
        }
        util::NormalizeIndices(fd_input.lhs);
        for (std::variant<std::string, model::Index>& s : fd_input.rhs) {
            s = std::visit(normalize_index, s);
        }
        util::NormalizeIndices(fd_input.rhs);
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(Option{&fd_input_, kFd, kDFd}
                           .SetNormalizeFunc(normalize_fd_input)
                           .SetValueCheck(not_empty));
}

void FDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kFd});
}

void FDVerifier::LoadDataInternal() {
    std::size_t const attr_num = input_table_->GetNumberOfColumns();
    std::vector<std::string> column_names;
    column_names.reserve(attr_num);
    for (size_t i = 0; i != attr_num; ++i) {
        column_names.push_back(input_table_->GetColumnName(i));
    }
    table_header_ = {input_table_->GetRelationName(), std::move(column_names)};

    // TODO: get rid of RelationalSchema in this class too.
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);
    input_table_->Reset();
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: FD verifying is meaningless.");
    }
    typed_relation_ =
            model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, is_null_equal_null_);
}

unsigned long long FDVerifier::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();

    std::vector<model::Index> lhs_indices = ConvertToIndexVector(fd_input_.lhs);
    std::vector<model::Index> rhs_indices = ConvertToIndexVector(fd_input_.rhs);
    stats_calculator_ =
            std::make_unique<StatsCalculator>(relation_, typed_relation_, lhs_indices, rhs_indices);

    VerifyFD(lhs_indices, rhs_indices);
    SortHighlightsByProportionDescending();
    stats_calculator_->PrintStatistics();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void FDVerifier::VerifyFD(std::vector<model::Index> const& lhs_indices,
                          std::vector<model::Index> const& rhs_indices) const {
    std::shared_ptr<model::PLI const> lhs_pli = relation_->CalculatePLI(lhs_indices);
    std::shared_ptr<model::PLI const> rhs_pli = relation_->CalculatePLI(rhs_indices);

    std::unique_ptr<model::PLI const> intersection_pli = lhs_pli->Intersect(rhs_pli.get());
    if (lhs_pli->GetNumCluster() == intersection_pli->GetNumCluster()) {
        return;
    }

    stats_calculator_->CalculateStatistics(lhs_pli.get(), rhs_pli.get());
}

void FDVerifier::SortHighlightsByProportionAscending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsByProportionAscending());
}

void FDVerifier::SortHighlightsByProportionDescending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsByProportionDescending());
}

void FDVerifier::SortHighlightsByNumAscending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsByNumAscending());
}

void FDVerifier::SortHighlightsByNumDescending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsByNumDescending());
}

void FDVerifier::SortHighlightsBySizeAscending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsBySizeAscending());
}

void FDVerifier::SortHighlightsBySizeDescending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(StatsCalculator::CompareHighlightsBySizeDescending());
}

void FDVerifier::SortHighlightsByLhsAscending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(stats_calculator_->CompareHighlightsByLhsAscending());
}

void FDVerifier::SortHighlightsByLhsDescending() const {
    assert(stats_calculator_);
    stats_calculator_->SortHighlights(stats_calculator_->CompareHighlightsByLhsDescending());
}

}  // namespace algos::fd_verifier
