#include "checks/explicitcapture.h"

#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchers.h>

namespace clang::tidy::misc {
using namespace ast_matchers;

void ExplicitLambdaCapturesCheck::registerMatchers(MatchFinder* Finder) {
    Finder->addMatcher(traverse(TK_IgnoreUnlessSpelledInSource, lambdaExpr().bind("lambda")), this);
}

void ExplicitLambdaCapturesCheck::check(MatchFinder::MatchResult const& Result) {
    auto expr = Result.Nodes.getNodeAs<LambdaExpr>("lambda");
    if (expr == nullptr || expr->implicit_captures().empty()) return;

    diag(expr->getCaptureDefaultLoc(), "This lambda must explicitly lists all of its captures");
}
};  // namespace clang::tidy::misc