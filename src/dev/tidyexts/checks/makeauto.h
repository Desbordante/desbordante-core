#pragma once

#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy::misc {

// Checks that the following
//    auto a = std::make_unique<int>(25);
// is used instead of
//    std::unique_ptr<int> a = std::make_unique<int>(25);
class MakeAutoCheck : public ClangTidyCheck {
public:
    MakeAutoCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {}

    void registerMatchers(ast_matchers::MatchFinder* Finder) override;

    void check(ast_matchers::MatchFinder::MatchResult const& Result) override;
};

}  // namespace clang::tidy::misc