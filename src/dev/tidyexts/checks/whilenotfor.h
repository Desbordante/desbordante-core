#pragma once

#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy::misc {

// Checks that the following
//    while(X) { Y }
// is used instead of
//    for (;X;) { Y }
class WhileNotForCheck : public ClangTidyCheck {
public:
    WhileNotForCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {}

    void registerMatchers(ast_matchers::MatchFinder* Finder) override;

    void check(ast_matchers::MatchFinder::MatchResult const& Result) override;
};

}  // namespace clang::tidy::misc