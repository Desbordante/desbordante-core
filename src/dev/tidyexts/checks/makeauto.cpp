#include "makeauto.h"

#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceLocation.h"

namespace clang::tidy::misc {
using namespace ast_matchers;

void MakeAutoCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
    Finder->addMatcher(
        traverse(
            TK_IgnoreUnlessSpelledInSource,
            varDecl(
                unless(hasType(autoType())),
                hasInitializer(callExpr(callee(functionDecl(anyOf(
                    hasName("std::make_any"),
                    hasName("std::make_heap"),
                    hasName("std::make_pair"),
                    hasName("std::make_tuple"),
                    hasName("std::make_shared"),
                    hasName("std::make_signed"),
                    hasName("std::make_unique"),
                    hasName("std::make_optional"),
                    hasName("std::make_unsigned"),
                    hasName("std::make_from_tuple"),
                    hasName("std::make_exception_ptr"),
                    hasName("std::make_move_iterator"),
                    hasName("std::make_index_sequence"),
                    hasName("std::make_integer_sequence"),
                    hasName("std::make_reverse_iterator")
                )))))
            )
        ).bind("errordecl"),
        this
    );
}

void MakeAutoCheck::check(ast_matchers::MatchFinder::MatchResult const& Result) {
    auto const* var = Result.Nodes.getNodeAs<VarDecl>("errordecl");
    if (var == nullptr) return;

    diag(var->getLocation(), "variable '%0' should be declared as auto")
            << var->getName()
            << FixItHint::CreateReplacement(
                       CharSourceRange::getTokenRange(
                               var->getTypeSourceInfo()->getTypeLoc().getSourceRange()),
                       "auto");
}
};  // namespace clang::tidy::misc