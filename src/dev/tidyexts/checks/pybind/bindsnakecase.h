#pragma once

#include <clang/AST/ExprCXX.h>
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
    bool IsSnakeCase(std::string const& in, bool caps);
    std::string ToSnakeCase(std::string const& in, bool caps);

    bool IsPascalCase(std::string const& in);
    std::string ToPascalCase(std::string const& in);

    void CheckChain(StringLiteral const* binding_name, bool should_be_caps,
                    std::string const& message);
    void CheckConstructor(StringLiteral const* binding_name, bool pascal,
                          std::string const& message);
};
};  // namespace clang::tidy::misc