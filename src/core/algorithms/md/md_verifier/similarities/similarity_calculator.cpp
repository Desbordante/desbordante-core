#include "algorithms/md/md_verifier/similarities/similarity_calculator.h"

#include "algorithms/md/md_verifier/md_verifier_column_match.h"
#include "model/types/types.h"

namespace algos::md {
model::md::Similarity SimilarityCalculator::Calculate(std::byte const* first_val,
                                                      std::byte const* second_val,
                                                      model::TypeId type_id,
                                                      std::shared_ptr<SimilarityMeasure> measure) {
    switch (measure->GetType()) {
        case SimilarityMeasureType::kNumericSimilarity:
            return CalculateNumericSimilarity(first_val, second_val, type_id,
                                              AsNumericMeasure(measure));

        case SimilarityMeasureType::kStringSimilarity:
            return CalculateStringSimilarity(first_val, second_val, type_id,
                                             AsStringMeasure(measure));

        default:
            throw std::runtime_error(
                    "Failed to calcutate similarity measure: unsupported similarity measure type "
                    "provided");
    }
}

model::md::Similarity SimilarityCalculator::CalculateNumericSimilarity(
        std::byte const* first_val, std::byte const* second_val, model::TypeId type_id,
        std::shared_ptr<NumericSimilarityMeasure> measure) {
    double first, second;
    switch (type_id) {
        case model::TypeId::kInt: {
            first = static_cast<model::md::DecisionBoundary>(
                    model::INumericType::GetValue<model::Int>(first_val));
            second = static_cast<model::md::DecisionBoundary>(
                    model::INumericType::GetValue<model::Int>(second_val));
            return (*measure)(first, second);
        }

        case model::TypeId::kDouble: {
            first = static_cast<model::md::DecisionBoundary>(
                    model::INumericType::GetValue<model::Double>(first_val));
            second = static_cast<model::md::DecisionBoundary>(
                    model::INumericType::GetValue<model::Double>(first_val));
            return (*measure)(first, second);
        }

        default:
            throw std::runtime_error(
                    "Failed to calcutate similarity measure: unsupported column type for numeric "
                    "similarity provided.");
    }
}

model::md::Similarity SimilarityCalculator::CalculateStringSimilarity(
        std::byte const* first_val, std::byte const* second_val, model::TypeId type_id,
        std::shared_ptr<StringSimilarityMeasure> measure) {
    std::string first, second;
    switch (type_id) {
        case model::TypeId::kInt: {
            model::IntType type;
            first = type.ValueToString(first_val);
            second = type.ValueToString(second_val);
            return (*measure)(first, second);
        }

        case model::TypeId::kDouble: {
            model::DoubleType type;
            first = type.ValueToString(first_val);
            second = type.ValueToString(second_val);
            return (*measure)(first, second);
        }

        case model::TypeId::kString: {
            model::StringType string_type = model::StringType();
            first = string_type.ValueToString(first_val);
            second = string_type.ValueToString(second_val);
            return (*measure)(first, second);
        }

        default:
            throw std::runtime_error(
                    "Failed to calcutate similarity measure: unsupported column type for numeric "
                    "similarity provided.");
    }
}
}  // namespace algos::md
