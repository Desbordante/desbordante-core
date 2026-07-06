#pragma once

#include "core/config/indices/type.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "pattern_info.h"

namespace algos::pattern_fd {

enum class TokenNGramType { kToken, kNGram };

using TokenToRows = std::unordered_map<TokenPatternInfo, std::vector<size_t>>;
using TokensMap = std::unordered_map<config::IndexType, TokenToRows>;

class Tokenizer {
private:
    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    TokensMap tokens_map_;  // maps of tokens for each column

    bool OneValueAttribute(model::TypedColumnData const& column);

    TokenNGramType TokenizeOrNGrams(model::TypedColumnData const& column);

    std::vector<TokenPatternInfo> GetTokenPatternInfo(std::string const& value);

    std::vector<TokenPatternInfo> GetNGramPatternInfo(std::string_view value, double avg_len);

public:
    Tokenizer(std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation)
        : typed_relation_(std::move(typed_relation)) {}

    void TokenizeColumn(config::IndexType column_index);

    TokensMap const& GetTokensMap() const {
        return tokens_map_;
    }
};

}  // namespace algos::pattern_fd