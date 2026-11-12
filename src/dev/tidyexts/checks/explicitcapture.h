#pragma once

#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy::misc {

// Checks that all variables in a lambda are explicitly captured
class ExplicitLambdaCapturesCheck : public ClangTidyCheck {
public:
    ExplicitLambdaCapturesCheck(StringRef Name, ClangTidyContext* Context)
        : ClangTidyCheck(Name, Context) {}

    void registerMatchers(ast_matchers::MatchFinder* Finder) override;

    void check(ast_matchers::MatchFinder::MatchResult const& Result) override;
};

}  // namespace clang::tidy::misc