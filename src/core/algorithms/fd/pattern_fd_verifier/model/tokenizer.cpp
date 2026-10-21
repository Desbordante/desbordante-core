#include "core/algorithms/fd/pattern_fd_verifier/model/tokenizer.h"

namespace algos::pattern_fd {

void Tokenizer::PrepareColumn(config::IndexType column_index) {
    if (column_modes_.contains(column_index)) return;

    auto const& column = typed_relation_->GetColumnData(column_index);
    size_t non_empty_count = 0;
    size_t total_token_count = 0;
    for (size_t row = 0; row < column.GetNumRows(); ++row) {
        std::string const value = column.GetDataAsString(row);
        if (value.empty()) continue;

        ++non_empty_count;
        std::sregex_token_iterator token(value.begin(), value.end(), delimiter_regex_, -1);
        for (std::sregex_token_iterator end; token != end; ++token) {
            if (token->length() != 0) ++total_token_count;
        }
    }

    column_modes_[column_index] = total_token_count > 2 * non_empty_count ? TokenNGramType::kToken
                                                                          : TokenNGramType::kNGram;
}

bool Tokenizer::Matches(config::IndexType column_index, TokenPatternInfo const& pattern,
                        std::string const& value) const {
    if (pattern.Token().empty()) return false;
    if (column_modes_.at(column_index) == TokenNGramType::kNGram) {
        return pattern.Position() <= value.size() &&
               value.compare(pattern.Position(), pattern.Token().size(), pattern.Token()) == 0;
    }

    std::sregex_token_iterator token(value.begin(), value.end(), delimiter_regex_, -1);
    size_t position = 0;
    std::string candidate;
    for (std::sregex_token_iterator end; token != end; ++token) {
        if (token->length() == 0) continue;
        if (position < pattern.Position()) {
            ++position;
            continue;
        }
        candidate += token->str();
        if (candidate == pattern.Token()) return true;
        if (candidate.size() >= pattern.Token().size()) return false;
    }
    return false;
}

}  // namespace algos::pattern_fd
