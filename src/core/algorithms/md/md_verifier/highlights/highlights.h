#pragma once

#include <sstream>

#include "core/algorithms/md/hymd/table_identifiers.h"
#include "core/algorithms/md/md.h"
#include "core/algorithms/md/similarity.h"
#include "core/config/indices/type.h"
#include "core/model/index.h"

namespace algos::md {
class MDHighlights {
public:
    struct Highlight {
        hymd::RecordIdentifier left_record_id;
        hymd::RecordIdentifier right_record_id;
        model::md::Similarity similarity;
        model::RhsSimilarityClassifierDesctription const& rhs_desc;

        Highlight(hymd::RecordIdentifier left_record_id, hymd::RecordIdentifier right_record_id,
                  model::md::Similarity similarity,
                  model::RhsSimilarityClassifierDesctription const& rhs_desc)
            : left_record_id(left_record_id),
              right_record_id(right_record_id),
              similarity(similarity),
              rhs_desc(rhs_desc) {}

        std::string ToString() const {
            std::stringstream ss;
            ss << "Rows (" << left_record_id << ", " << right_record_id << ") have similarity "
               << similarity << ", while dependency states "
               << rhs_desc.column_match_description.column_match_name << '('
               << rhs_desc.column_match_description.left_column_description.column_name << ", "
               << rhs_desc.column_match_description.right_column_description.column_name
               << ")>=" << rhs_desc.decision_boundary;
            return ss.str();
        };

        std::string ToStringIndexes() const {
            std::stringstream ss;
            ss << "Rows (" << left_record_id << ", " << right_record_id << ") have similarity "
               << similarity << ", while dependency states "
               << rhs_desc.column_match_description.column_match_name << '('
               << rhs_desc.column_match_description.left_column_description.column_index << ", "
               << rhs_desc.column_match_description.right_column_description.column_index
               << ")>=" << rhs_desc.decision_boundary;
            return ss.str();
        };
    };

private:
    model::RhsSimilarityClassifierDesctription rhs_desc_;
    std::vector<Highlight> highlights_;

public:
    MDHighlights(model::RhsSimilarityClassifierDesctription const& rhs_desc)
        : rhs_desc_(rhs_desc) {}

    std::vector<Highlight> const& GetHighlights() const {
        return highlights_;
    }

    void AddHighlight(hymd::RecordIdentifier left_record_id, hymd::RecordIdentifier right_record_id,
                      model::md::Similarity similarity) {
        highlights_.emplace_back(left_record_id, right_record_id, similarity, rhs_desc_);
    }
};
}  // namespace algos::md
