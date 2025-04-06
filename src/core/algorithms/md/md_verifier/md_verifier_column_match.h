#pragma once

#include <memory>
#include <sstream>

#include "algorithms/md/column_match.h"
#include "algorithms/md/md_verifier/similarities/similarities.h"
#include "algorithms/md/similarity.h"

namespace algos::md {
struct MDVerifierColumnMatch : model::md::ColumnMatch {
    std::shared_ptr<algos::md::SimilarityMeasure> measure;

    MDVerifierColumnMatch() : ColumnMatch(0, 0, "") {}

    MDVerifierColumnMatch(model::Index left_col_index, model::Index right_col_index,
                          std::shared_ptr<algos::md::SimilarityMeasure> measure)
        : ColumnMatch(left_col_index, right_col_index, measure->GetName()), measure(measure) {}

    std::string ToString() const {
        std::stringstream ss;
        ss << name << "(" << left_col_index << ", " << right_col_index << ")";
        return ss.str();
    };

    model::md::ColumnMatch ToStandardColumnMatch() const {
        return ColumnMatch(left_col_index, right_col_index, name);
    }
};
}  // namespace algos::md
