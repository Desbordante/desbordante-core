#include "tokenizer.h"

#include <regex>

#include "model/types/type.h"

namespace algos::pattern_fd {

TokenNGramType Tokenizer::TokenizeOrNGrams(model::TypedColumnData const& column) {
    std::unordered_map<int, int>
            token_counts;  // token number -> number of lines with this token number
    int non_empty_count = 0;
    config::IndexType rows_num = column.GetNumRows();

    static std::regex token_regex(R"((\d*\.\d+|\W))");  // split by special symbols

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

void Tokenizer::TokenizeColumn(config::IndexType column_index) {
    if (tokens_map_.count(column_index) != 0) {
        return;  // skip tokenized columns
    }

    size_t rows_num = typed_relation_->GetNumRows();
    static std::regex token_regex(R"((\d*\.\d+|\W))");

    model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);

    if (column.IsNumeric()) {
        return;  // skip numeric columns
    }

    std::vector<std::string> values;
    values.reserve(rows_num);
    std::unordered_set<std::string_view> unique_values;
    std::unordered_map<int, int> token_counts;
    size_t total_len = 0;
    int non_empty_count = 0;
    int total_token_count = 0;

    for (config::IndexType row_index = 0; row_index < rows_num; row_index++) {
        values.push_back(column.GetDataAsString(row_index));
        std::string const& value = values.back();

        if (value.empty()) {
            continue;
        }

        if (unique_values.size() <= 1) {
            unique_values.insert(value);
        }

        std::sregex_token_iterator token_iter(value.begin(), value.end(), token_regex, -1);
        int num_tokens = 0;
        for (std::sregex_token_iterator end; token_iter != end; ++token_iter) {
            if (!token_iter->str().empty()) {
                num_tokens++;
            }
        }
        token_counts[num_tokens]++;
        total_token_count += num_tokens;

        total_len += value.size();
        non_empty_count++;
    }

    if (unique_values.size() <= 1) {
        return;
    }

    TokenNGramType token_type = (total_token_count > 2 * non_empty_count) ? TokenNGramType::kToken
                                                                          : TokenNGramType::kNGram;

    double avg_len = (non_empty_count > 0) ? static_cast<double>(total_len) / non_empty_count : 0.0;

    for (config::IndexType row_index = 0; row_index < rows_num; row_index++) {
        std::string const value = column.GetDataAsString(row_index);
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

std::vector<TokenPatternInfo> Tokenizer::GetNGramPatternInfo(std::string_view value,
                                                             double avg_len) {
    std::vector<TokenPatternInfo> patterns;
    if (value.empty()) {
        return patterns;
    }
    if (static_cast<int>(avg_len) > 30) {
        patterns.push_back(TokenPatternInfo(std::string(value), 0));
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
            std::string_view ngram = value.substr(j, n_gram_len);
            patterns.push_back(TokenPatternInfo(std::string(ngram), j));
        }
    }
    return patterns;
}

std::vector<TokenPatternInfo> Tokenizer::GetTokenPatternInfo(std::string const& value) {
    std::vector<TokenPatternInfo> patterns;
    static std::regex const kRegexSplit(R"((\d*\.\d+|\W))");
    if (value.empty()) {
        return patterns;
    }

    std::sregex_token_iterator it(value.begin(), value.end(), kRegexSplit, -1);
    std::vector<std::string> tokens;
    for (std::sregex_token_iterator end; it != end; ++it) {
        if (it->length() > 0) {
            tokens.push_back(it->str());
        }
    }

    int len_tokens = static_cast<int>(tokens.size());
    for (int j = 0; j < len_tokens; ++j) {
        std::string gram;
        for (int k = 0; j + k < len_tokens; ++k) {
            gram.append(tokens[j + k]);

            if (gram.size() <= 4 && value.size() > 1) {
                continue;
            }
            patterns.emplace_back(gram, j);
        }
    }
    return patterns;
}

}  // namespace algos::pattern_fd