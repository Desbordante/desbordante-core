#include "pythonrepr.h"

#include <clang/ASTMatchers/ASTMatchers.h>

namespace clang::tidy::misc {
using namespace ast_matchers;

CXXConstructExpr const* PythonReprCheck::ResolveChainRoot(Expr const* receiver) {
    while (receiver != nullptr) {
        receiver = receiver->IgnoreUnlessSpelledInSource()->IgnoreParens();
        if (auto const* call = dyn_cast<CXXMemberCallExpr>(receiver)) {
            receiver = call->getImplicitObjectArgument();
            continue;
        }
        if (auto const* cast = dyn_cast<CXXFunctionalCastExpr>(receiver);
            cast != nullptr && cast->getCastKind() == CK_ConstructorConversion) {
            receiver = cast->getSubExpr();
            continue;
        }
        auto const* constructor = dyn_cast<CXXConstructExpr>(receiver);
        if (constructor != nullptr &&
            constructor->getConstructor()->getParent()->getQualifiedNameAsString() ==
                    "pybind11::class_") {
            return constructor;
        }
        break;
    }
    return nullptr;
}

StringRef PythonReprCheck::GetBindingName(CXXMemberCallExpr const* call) {
    auto const* method = call->getMethodDecl();
    if (method == nullptr || method->getIdentifier() == nullptr || method->getName() != "def" ||
        method->getParent()->getQualifiedNameAsString() != "pybind11::class_" ||
        call->getNumArgs() == 0) {
        return {};
    }

    auto const* name = dyn_cast<StringLiteral>(call->getArg(0)->IgnoreUnlessSpelledInSource());
    return name != nullptr && name->getCharByteWidth() == 1 ? name->getString() : StringRef{};
}

void PythonReprCheck::registerMatchers(MatchFinder* Finder) {
    Finder->addMatcher(
            traverse(TK_IgnoreUnlessSpelledInSource,
                     cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(
                                              cxxRecordDecl(hasName("::pybind11::class_"))))))
                             .bind("constructor")),
            this);
    Finder->addMatcher(traverse(TK_IgnoreUnlessSpelledInSource, cxxMemberCallExpr().bind("call")),
                       this);
}

void PythonReprCheck::check(MatchFinder::MatchResult const& Result) {
    if (auto const* constructor = Result.Nodes.getNodeAs<CXXConstructExpr>("constructor")) {
        chains_.try_emplace(constructor);
        return;
    }

    auto const* call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("call");
    if (auto const* root = ResolveChainRoot(call->getImplicitObjectArgument())) {
        chains_[root].push_back(call);
    }
}

void PythonReprCheck::onEndOfTranslationUnit() {
    for (auto const& [root, calls] : chains_) {
        bool has_repr = false;
        bool has_str = false;
        for (auto const* call : calls) {
            StringRef name = GetBindingName(call);
            has_repr |= name == "__repr__";
            has_str |= name == "__str__";
        }

        if (!has_repr || !has_str) {
            diag(root->getBeginLoc(),
                 "Python class binding is missing %select{.def(\"__repr__\")|"
                 ".def(\"__str__\")|.def(\"__repr__\") and .def(\"__str__\")}0")
                    << (has_repr ? 1 : (has_str ? 0 : 2));
        }
    }
    chains_.clear();
}
}  // namespace clang::tidy::misc
