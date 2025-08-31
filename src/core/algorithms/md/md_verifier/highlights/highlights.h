#pragma once

#include <sstream>

#include "algorithms/md/md.h"
#include "algorithms/md/md_verifier/validation/records_pairs.h"
#include "algorithms/md/similarity.h"
#include "config/indices/type.h"
#include "model/index.h"

namespace algos::md {
class MDHighlights {
public:
    struct Highlight {
        model::Index left_table_row;
        model::Index right_table_row;
        model::md::Similarity similarity;
        model::RhsSimilarityClassifierDesctription rhs_desc;

        Highlight(model::Index left_table_row, model::Index right_table_row,
                  model::RhsSimilarityClassifierDesctription rhs_desc,
                  model::md::Similarity similarity)
            : left_table_row(left_table_row),
              right_table_row(right_table_row),
              similarity(similarity),
              rhs_desc(rhs_desc) {}

        std::string ToString() const {
            std::stringstream ss;
            ss << "Rows " << left_table_row << " of the left table and " << right_table_row
               << " of the right table have similarity " << similarity
               << " and violate right-hand side column similarity classifier "
               << rhs_desc.column_match_description.column_match_name << '('
               << rhs_desc.column_match_description.left_column_description.column_name << ", "
               << rhs_desc.column_match_description.right_column_description.column_name
               << ")>=" << rhs_desc.decision_boundary;
            return ss.str();
        };

        std::string ToStringIndexes() const {
            std::stringstream ss;
            ss << "Rows " << left_table_row << " of the left table and " << right_table_row
               << " of the right table have similarity " << similarity
               << " and violate right-hand side column similarity classifier "
               << rhs_desc.column_match_description.column_match_name << '('
               << rhs_desc.column_match_description.left_column_description.column_index << ", "
               << rhs_desc.column_match_description.right_column_description.column_index
               << ")>=" << rhs_desc.decision_boundary;
            return ss.str();
        };
    };

private:
    std::vector<Highlight> highlights_;

public:
    MDHighlights() {}

    MDHighlights(std::vector<Highlight> highlights) : highlights_(std::move(highlights)) {}

    std::vector<Highlight> const& GetHighlights() const {
        return highlights_;
    }

    static MDHighlights CreateFrom(model::RhsSimilarityClassifierDesctription rhs_desc,
                                   RecordsPairsSet const& rows_pairs,
                                   RecordsPairToSimilarityMap const& rows_to_similarity);
};
}  // namespace algos::md
