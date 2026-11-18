#pragma once

#include <clang/AST/ExprCXX.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/SmallVector.h>

#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy::misc {
class PythonReprCheck : public ClangTidyCheck {
public:
    PythonReprCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {}

    void registerMatchers(ast_matchers::MatchFinder* Finder) override;

    void check(ast_matchers::MatchFinder::MatchResult const& Result) override;

    void onEndOfTranslationUnit() override;

private:
    static CXXConstructExpr const* ResolveChainRoot(Expr const* receiver);
    static StringRef GetBindingName(CXXMemberCallExpr const* call);

    llvm::MapVector<CXXConstructExpr const*, llvm::SmallVector<CXXMemberCallExpr const*>> chains_;
};
}  // namespace clang::tidy::misc
