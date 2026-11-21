#pragma once

#include <regex>
#include <unordered_map>
#include <utility>

#include "core/config/indices/type.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "pattern_info.h"

namespace algos::pattern_fd {

enum class TokenNGramType { kToken, kNGram };

class Tokenizer {
private:
    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::regex delimiter_regex_{R"((\d*\.\d+|\W))"};
    std::unordered_map<config::IndexType, TokenNGramType> column_modes_;

public:
    Tokenizer(std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation)
        : typed_relation_(std::move(typed_relation)) {}

    void PrepareColumn(config::IndexType column_index);

    bool Matches(config::IndexType column_index, TokenPatternInfo const& pattern,
                 std::string const& value) const;
};

}  // namespace algos::pattern_fd
