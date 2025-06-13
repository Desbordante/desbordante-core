#include "tokenizer.h"

#include <regex>

#include "model/types/type.h"

namespace algos::pattern_fd {

TokenNGramType Tokenizer::TokenizeOrNGrams(model::TypedColumnData const& column) {
    std::unordered_map<int, int>
            token_counts;  // token number -> number of lines with this token number
    int non_empty_count = 0;
    config::IndexType rows_num = column.GetNumRows();

    std::regex token_regex(R"((\d*\.\d+|\W))");  // split by special symbols

    for (config::IndexType row_index = 0; row_index < rows_num; row_index++) {
        std::string value = column.GetDataAsString(row_index);
        if (value.empty()) continue;
        non_empty_count++;
        std::sregex_token_iterator token_iter(value.begin(), value.end(), token_regex, -1);
        std::sregex_token_iterator token_end;
        std::vector<std::string> tokens;
        for (; token_iter != token_end; ++token_iter) {
            if (!token_iter->str().empty()) {
                tokens.push_back(*token_iter);
            }
        }

        int token_count = static_cast<int>(tokens.size());
        token_counts[token_count]++;
    }

    int total_token_count = 0;
    for (auto const& [token_count, row_count] : token_counts) {
        total_token_count += token_count * row_count;
    }

    if (total_token_count > 2 * non_empty_count) {
        return TokenNGramType::kToken;
    } else {
        return TokenNGramType::kNGram;
    }
}

void Tokenizer::TokenizeColumns() {
    tokens_map_.clear();

    model::ColumnIndex columns_num = typed_relation_->GetNumColumns();
    size_t rows_num = typed_relation_->GetNumRows();

    for (model::ColumnIndex column_index = 0; column_index < columns_num; column_index++) {
        model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);

        if (column.IsNumeric() || OneValueAttribute(column)) {
            continue;  // skip numeric or one value columns
        }
        TokenNGramType token_type = TokenizeOrNGrams(column);

        double avg_len = 0.0;
        size_t non_empty_count = 0;
        size_t total_len = 0;
        for (config::IndexType row_index = 0; row_index < rows_num; row_index++) {
            std::string value = column.GetDataAsString(row_index);
            if (!value.empty()) {
                total_len += value.size();
                non_empty_count++;
            }
        }
        if (non_empty_count > 0) {
            avg_len = static_cast<double>(total_len) / non_empty_count;
        }

        for (config::IndexType row_index = 0; row_index < rows_num; row_index++) {
            std::string value = column.GetDataAsString(row_index);
            if (value.empty()) continue;
            std::vector<TokenPatternInfo> patterns_info;
            if (token_type == TokenNGramType::kNGram) {
                patterns_info = GetNGramPatternInfo(value, avg_len);
            } else {
                patterns_info = GetTokenPatternInfo(value);
            }
            for (auto const& pattern_info : patterns_info) {
                tokens_map_[column_index][pattern_info].push_back(row_index);
            }
        }
    }
}

bool Tokenizer::OneValueAttribute(model::TypedColumnData const& column) {
    std::set<std::string> values;
    config::IndexType rows_num = column.GetNumRows();
    for (config::IndexType row_index = 0; row_index < rows_num; ++row_index) {
        std::string v = column.GetDataAsString(row_index);
        if (v.size() != 0) {
            values.insert(v);
        }
        if (values.size() > 1) {
            return false;
        }
    }
    return true;
}

std::vector<TokenPatternInfo> Tokenizer::GetNGramPatternInfo(std::string const& value,
                                                             double avg_len) {
    std::vector<TokenPatternInfo> patterns;
    if (value.empty()) {
        return patterns;
    }
    if (static_cast<int>(avg_len) > 30) {
        patterns.push_back(TokenPatternInfo(value, 0));
        return patterns;
    }
    int value_length = static_cast<int>(value.size());
    int min_n_gram_len = std::min(3, value_length) - 1;
    for (int n_gram_len = value_length; n_gram_len > min_n_gram_len; --n_gram_len) {
        std::vector<int> req_pos;
        for (int j = 0; j <= value_length - n_gram_len; ++j) {
            req_pos.push_back(j);
        }
        for (int j : req_pos) {
            if (j < 0 || j >= value_length || j + n_gram_len > value_length) {
                continue;
            }
            std::string ngram = value.substr(j, n_gram_len);
            patterns.push_back(TokenPatternInfo(ngram, j));
        }
    }
    return patterns;
}

std::vector<TokenPatternInfo> Tokenizer::GetTokenPatternInfo(std::string const& value) {
    std::vector<TokenPatternInfo> patterns;
    std::regex const regex_split(R"((\d*\.\d+|\W))");
    if (value.empty()) {
        return patterns;
    }
    std::sregex_token_iterator it(value.begin(), value.end(), regex_split, -1);
    std::sregex_token_iterator end;
    std::vector<std::string_view> tokens;
    for (; it != end; ++it) {
        if (!it->str().empty()) {
            auto offset = std::distance(value.begin(), it->first);
            tokens.emplace_back(value.data() + offset, it->length());
        }
    }

    int len_tokens = static_cast<int>(tokens.size());
    for (int k = len_tokens - 1; k >= 0; --k) {
        std::vector<int> req_pos;
        for (int j = 0; j < len_tokens - k; ++j) {
            req_pos.push_back(j);
        }

        for (int j : req_pos) {
            if (j < 0 || j + k >= len_tokens) {
                continue;
            }

            std::string gram;
            for (int jj = 0; jj <= k; ++jj) {
                gram += tokens[j + jj];
            }
            if (gram.size() <= 4 && value.size() > 1) {
                continue;
            }
            patterns.push_back(TokenPatternInfo(gram, j));
        }
    }
    return patterns;
}

}  // namespace algos::pattern_fd