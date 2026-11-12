#pragma once

#include <clang/AST/Type.h>
#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy::misc {

// Forces the use of snake_case when writing binds for Python
class BindSnakeCaseCheck : public ClangTidyCheck {
public:
    BindSnakeCaseCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {}

    void registerMatchers(ast_matchers::MatchFinder* Finder) override;

    void check(ast_matchers::MatchFinder::MatchResult const& Result) override;

private:
    bool IsSnakeCase(const std::string& in);
    std::string ToSnakeCase(const std::string& in);
};

}  // namespace clang::tidy::misc