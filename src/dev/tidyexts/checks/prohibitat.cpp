#include "prohibitat.h"

#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <optional>
#include <stdexcept>

namespace clang::tidy::misc {
using namespace ast_matchers;

void ProhibitAtCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
    Finder->addMatcher(traverse(TK_IgnoreUnlessSpelledInSource,
                                cxxMemberCallExpr(callee(cxxMethodDecl(hasName("at"))),
                                                  hasArgument(0, expr().bind("idxarg")))
                                        .bind("callexpr")),
                       this);
}

void ProhibitAtCheck::check(MatchFinder::MatchResult const& Result) {
    auto const* call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("callexpr");
    if (call == nullptr) return;
    auto const* idx_arg = Result.Nodes.getNodeAs<Expr>("idxarg");
    if (idx_arg == nullptr) return;

    auto const container_type = Result.Context->getRecordType(call->getMethodDecl()->getParent());
    auto const opt_type = ResolveContainerKind(container_type);
    if (!opt_type.has_value()) return;
    auto const type = opt_type.value();

    auto const* sm = Result.SourceManager;

    std::string arg_expr =
            Lexer::getSourceText(
                    CharSourceRange::getTokenRange(idx_arg->getBeginLoc(), idx_arg->getEndLoc()),
                    *sm, Result.Context->getLangOpts())
                    .str();
    auto const* me = dyn_cast<MemberExpr>(call->getCallee()->IgnoreParenImpCasts());
    if (me == nullptr) return;
    std::string fix = GenerateFixText(type, arg_expr, me->isArrow());

    diag(me->getOperatorLoc(), ".at() is prohibited for STL containers")
            << FixItHint::CreateReplacement(
                       CharSourceRange::getCharRange(me->getOperatorLoc(), call->getRParenLoc()),
                       fix);
}

std::optional<ProhibitAtCheck::ContainerKind> ProhibitAtCheck::ResolveContainerKind(
        QualType const& type) {
    auto const* rd = type->getAsCXXRecordDecl();
    if (rd == nullptr) return std::nullopt;
    std::string const name = rd->getQualifiedNameAsString();

    if (name == "std::vector" || name == "std::array") {
        return ContainerKind::kVectorLike;
    } else if (name == "std::map" || name == "std::unordered_map") {
        return ContainerKind::kMapLike;
    }

    return std::nullopt;
}

std::string ProhibitAtCheck::GenerateFixText(ContainerKind kind, std::string const& idx_var,
                                           bool is_arrow) {
    switch (kind) {
        case ContainerKind::kVectorLike:
            return is_arrow ? "->operator[](" + idx_var + ")" : "[" + idx_var + "]";
        case ContainerKind::kMapLike:
            return (is_arrow ? "->find(" : ".find(") + idx_var + ")->second";
        default:
            throw std::runtime_error("unreachable");
    }
}
};  // namespace clang::tidy::misc