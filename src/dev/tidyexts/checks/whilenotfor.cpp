#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Stmt.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Lex/Lexer.h"

#include "checks/whilenotfor.h"

namespace clang::tidy::misc {
using namespace ast_matchers;

void WhileNotForCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
    Finder->addMatcher(
        traverse(
            TK_IgnoreUnlessSpelledInSource,
            forStmt(
                allOf(
                    unless(hasIncrement(stmt())),
                    unless(hasLoopInit(stmt())),
                    hasCondition(stmt())
                )
            ).bind("for")
        ),
        this
    );
}

void WhileNotForCheck::check(const ast_matchers::MatchFinder::MatchResult &Result) {
    const auto* for_loop = Result.Nodes.getNodeAs<ForStmt>("for");
    if (for_loop == nullptr) return;

    auto header_range = CharSourceRange::getTokenRange(for_loop->getForLoc(), for_loop->getRParenLoc());

    const auto* sm = Result.SourceManager;
    std::string replacement =
            "while (" +
            Lexer::getSourceText(
                    CharSourceRange::getTokenRange(for_loop->getCond()->getSourceRange()), *sm,
                    Result.Context->getLangOpts())
                    .str() +
            ")";

    diag(for_loop->getForLoc(), "this loop can be replaced with while")
        << FixItHint::CreateReplacement(
            header_range,
            replacement
        );
}
};  // namespace clang::tidy::misc