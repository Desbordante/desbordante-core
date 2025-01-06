#include "algorithms/md/md_verifier/highlights/highlights.h"

namespace algos::md {
std::string MDHighlights::HighlightRecordAsString(HighlightRecord highlight) {
    using namespace model;
    std::string result = "Rows " + std::to_string(highlight.rows.first) + " and " +
                         std::to_string(highlight.rows.second) + " violate MD in column " +
                         std::to_string(highlight.column) + ": \"" + highlight.first_value +
                         "\" and \"" + highlight.second_value + "\" have similarity " +
                         std::to_string(highlight.similarity) + " with decision boundary " +
                         std::to_string(highlight.decision_boundary);

    return result;
}

std::vector<std::string> MDHighlights::GetHighlightsAsStrings() const {
    std::vector<std::string> highlights_strings;
    highlights_strings.reserve(highlights.size());

    for (auto const& highlight : highlights) {
        highlights_strings.push_back(HighlightRecordAsString(highlight));
    }

    return highlights_strings;
}

void MDHighlights::AddHighlight(HighlightRecord record) {
    highlights.push_back(record);
}

void MDHighlights::Reset() {
    highlights.clear();
}
}  // namespace algos::md