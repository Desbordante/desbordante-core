#pragma once

#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"
#include "algorithms/md/hymd/preprocessing/similarity_measure/monge_elkan_metric.h"
#include "algorithms/md/hymd/similarity_measure_creator.h"
#include "config/exceptions.h"
#include "model/types/string_type.h"

namespace algos::hymd::preprocessing::similarity_measure {

inline std::vector<std::string> Tokenize(std::string const& text) {
    std::istringstream iss(text);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

class MongeElkanSimilarityMeasure : public ImmediateSimilarityMeasure {
    static constexpr auto kName = "monge_elkan_similarity";

public:
    class Creator final : public SimilarityMeasureCreator {
        model::md::DecisionBoundary const min_sim_;
        std::size_t const size_limit_;

    public:
        Creator(ColumnIdentifier column1_identifier, ColumnIdentifier column2_identifier,
                model::md::DecisionBoundary min_sim = 0.7, std::size_t size_limit = 0)
            : SimilarityMeasureCreator(kName, std::move(column1_identifier),
                                       std::move(column2_identifier)),
              min_sim_(min_sim),
              size_limit_(size_limit) {
            if (!(0.0 <= min_sim_ && min_sim_ <= 1.0)) {
                throw config::ConfigurationError("Minimum similarity out of range");
            }
        }

        std::unique_ptr<SimilarityMeasure> MakeMeasure(
                util::WorkerThreadPool* thread_pool) const final {
            return std::make_unique<MongeElkanSimilarityMeasure>(min_sim_, thread_pool,
                                                                 size_limit_);
        }
    };

    MongeElkanSimilarityMeasure(model::md::DecisionBoundary min_sim, util::WorkerThreadPool* pool,
                                std::size_t size_limit)
        : ImmediateSimilarityMeasure(
                  std::make_unique<model::StringType>(), true,
                  [min_sim](std::byte const* l, std::byte const* r) {
                      std::string const& left = model::Type::GetValue<std::string>(l);
                      std::string const& right = model::Type::GetValue<std::string>(r);
                      auto left_tokens = Tokenize(left);
                      auto right_tokens = Tokenize(right);
                      Similarity sim = MongeElkan(left_tokens, right_tokens);
                      return sim < min_sim ? kLowestBound : sim;
                  },
                  pool, size_limit) {}
};
}  // namespace algos::hymd::preprocessing::similarity_measure
