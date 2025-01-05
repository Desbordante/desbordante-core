#include "algorithms/md/md_verifier/highlights/highlights.h"

namespace algos::md {
std::string MDHighlights::HighlightRecordAsString(HighlightRecord highlight) {
    using namespace model;
    std::string result = "Rows " + std::to_string(highlight.rows.first) + " and " +
                         std::to_string(highlight.rows.second) + " violate MD in column " +
                         std::to_string(highlight.info.column) + ": ";

    switch (highlight.info.type_id) {
        case TypeId::kInt: {
            result += "\"" +
                      std::to_string(INumericType::GetValue<Int>(highlight.info.first_value)) +
                      "\" and \"" +
                      std::to_string(INumericType::GetValue<Int>(highlight.info.second_value)) +
                      "\" ";
        } break;

        case TypeId::kDouble: {
            result += "\"" +
                      std::to_string(INumericType::GetValue<Double>(highlight.info.first_value)) +
                      "\" and \"" +
                      std::to_string(INumericType::GetValue<Double>(highlight.info.second_value)) +
                      "\" ";
        } break;

        case TypeId::kString: {
            auto string_type = StringType();
            result += "\"" + string_type.ValueToString(highlight.info.first_value) + "\" and \"" +
                      string_type.ValueToString(highlight.info.second_value) + "\" ";

        } break;

        default:
            assert(false);
    }

    result += "have similarity " + std::to_string(highlight.info.similarity) +
              " with decision boundary " + std::to_string(highlight.info.decision_boundary);

    return result;
}

std::vector<std::string> MDHighlights::AsStrings() const {
    std::vector<std::string> highlights_strings;
    highlights_strings.reserve(highlights.size());

    for (auto const& highlight : highlights) {
        highlights_strings.push_back(HighlightRecordAsString(highlight));
    }

    return highlights_strings;
}

void MDHighlights::AddHighlight(std::pair<int, int> rows, HighlightRecordInfo info) {
    highlights.push_back({rows, info});
}

void MDHighlights::Reset() {
    highlights.clear();
}
}  // namespace algos::md