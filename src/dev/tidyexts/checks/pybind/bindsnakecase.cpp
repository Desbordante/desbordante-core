#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/CharInfo.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>

#include "bindsnakecase.h"
#include "common.h"

namespace clang::tidy::misc {
using namespace ast_matchers;

void BindSnakeCaseCheck::registerMatchers(MatchFinder *Finder) {
    Finder->addMatcher(
        traverse(
            TK_IgnoreUnlessSpelledInSource,
            cxxMemberCallExpr(
                callee(cxxMethodDecl(hasName("def"))),
                hasArgument(0, stringLiteral().bind("binding_name"))
            ).bind("callexpr")
        ),
        this
    );
}

void BindSnakeCaseCheck::check(const MatchFinder::MatchResult &Result) {
    const auto* call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("callexpr");
    const auto* binding_name = Result.Nodes.getNodeAs<StringLiteral>("binding_name");
    if (call == nullptr || binding_name == nullptr) return;
    if (IsSnakeCase(binding_name->getString().str())) return;
    if (!IsPybindClass(call->getImplicitObjectArgument()->getType())) return;
    std::string snake = ToSnakeCase(binding_name->getString().str());

    diag(binding_name->getBeginLoc(), "Method names for Python classes should be in snake_case")
        << FixItHint::CreateReplacement(
            CharSourceRange::getTokenRange(binding_name->getBeginLoc(), binding_name->getEndLoc()),
            '"' + snake + '"'
        );
}

bool BindSnakeCaseCheck::IsSnakeCase(const std::string &in) {
    for (char ch : in) {
        // according to PEP8 function names with uppercase letters are strictly prohibited with no exceptions
        // see https://peps.python.org/pep-0008/#function-and-variable-names
        if (isUppercase(ch)) return false;
    }
    return true;
}

std::string BindSnakeCaseCheck::ToSnakeCase(const std::string &in) {
    std::string out;

    for (std::string::size_type i = 0; i < in.length(); i++) {
        if (isUppercase(in[i])) {
            if (i != 0) out.push_back('_');
            out.push_back(toLowercase(in[i]));
        }
        else out.push_back(in[i]);
    }

    return out;
}
};