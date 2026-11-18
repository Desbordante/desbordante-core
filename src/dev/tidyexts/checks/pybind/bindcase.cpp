#include "bindcase.h"

#include <cctype>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/CharInfo.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>

namespace clang::tidy::misc {
using namespace ast_matchers;

void BindCaseCheck::registerMatchers(MatchFinder* Finder) {
    auto binding_name = traverse(
            TK_AsIs, ignoringImplicit(anyOf(
                             stringLiteral().bind("binding_name"),
                             cxxConstructExpr(hasArgument(
                                     0, ignoringImpCasts(stringLiteral().bind("binding_name")))))));
    Finder->addMatcher(
            traverse(TK_IgnoreUnlessSpelledInSource,
                     cxxMemberCallExpr(callee(cxxMethodDecl(hasName("def"))),
                                       on(hasType(cxxRecordDecl(hasName("pybind11::class_")))),
                                       hasArgument(0, binding_name))
                             .bind("defcall")),
            this);

    Finder->addMatcher(
            traverse(TK_IgnoreUnlessSpelledInSource,
                     cxxMemberCallExpr(
                             callee(cxxMethodDecl(anyOf(hasName("def_readonly_static"),
                                                        hasName("def_property_readonly_static")))),
                             on(hasType(cxxRecordDecl(hasName("pybind11::class_")))),
                             hasArgument(0, binding_name))
                             .bind("constdefcall")),
            this);

    Finder->addMatcher(
            traverse(TK_IgnoreUnlessSpelledInSource,
                     cxxMemberCallExpr(on(hasType(cxxRecordDecl(hasName("pybind11::module_")))),
                                       callee(cxxMethodDecl(hasName("attr"))),
                                       hasArgument(0, binding_name))
                             .bind("attrcall")),
            this);

    Finder->addMatcher(
            traverse(TK_IgnoreUnlessSpelledInSource,
                     cxxConstructExpr(hasDeclaration(cxxConstructorDecl(
                                              ofClass(cxxRecordDecl(hasName("pybind11::class_"))))),
                                      hasArgument(0, binding_name))
                             .bind("constructor")),
            this);

    Finder->addMatcher(
            traverse(TK_IgnoreUnlessSpelledInSource,
                     cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(
                                              cxxRecordDecl(hasName("pybind11::module_"))))),
                                      hasArgument(0, binding_name))
                             .bind("moduleconstructor")),
            this);
}

void BindCaseCheck::CheckChain(StringLiteral const* binding_name, bool should_be_caps,
                               std::string const& message) {
    if (IsSnakeCase(binding_name->getString().str(), should_be_caps)) return;
    std::string snake = ToSnakeCase(binding_name->getString().str(), should_be_caps);

    diag(binding_name->getBeginLoc(), message) << FixItHint::CreateReplacement(
            CharSourceRange::getTokenRange(binding_name->getBeginLoc(), binding_name->getEndLoc()),
            '"' + snake + '"');
}

void BindCaseCheck::CheckConstructor(StringLiteral const* binding_name, bool pascal,
                                     std::string const& message) {
    std::string name = binding_name->getString().str();
    if (pascal && IsPascalCase(name)) return;
    if (!pascal && IsSnakeCase(name, false)) return;
    std::string correct_case = pascal ? ToPascalCase(name) : ToSnakeCase(name, false);

    diag(binding_name->getBeginLoc(), message) << FixItHint::CreateReplacement(
            CharSourceRange::getTokenRange(binding_name->getBeginLoc(), binding_name->getEndLoc()),
            '"' + correct_case + '"');
}

void BindCaseCheck::check(MatchFinder::MatchResult const& Result) {
    auto const* binding_name = Result.Nodes.getNodeAs<StringLiteral>("binding_name");
    auto const* defcall = Result.Nodes.getNodeAs<CXXMemberCallExpr>("defcall");
    auto const* constdef = Result.Nodes.getNodeAs<CXXMemberCallExpr>("constdefcall");
    auto const* attrcall = Result.Nodes.getNodeAs<CXXMemberCallExpr>("attrcall");
    auto const* constructor = Result.Nodes.getNodeAs<CXXConstructExpr>("constructor");
    auto const* moduleconstructor = Result.Nodes.getNodeAs<CXXConstructExpr>("moduleconstructor");

    if (binding_name == nullptr) return;

    if (defcall != nullptr)
        CheckChain(binding_name, false, "Method names for Python classes must be in snake_case");
    if (constdef != nullptr)
        CheckChain(binding_name, true,
                   "Constants for Python classes must be in SCREAMING_SNAKE_CASE");
    if (attrcall != nullptr)
        CheckChain(binding_name, true,
                   "Constants in Python modules must be in SCREAMING_SNAKE_CASE");
    if (constructor != nullptr)
        CheckConstructor(binding_name, true, "Python class names must be in CamelCase");
    if (moduleconstructor != nullptr)
        CheckConstructor(binding_name, false, "Module names must be in snake_case");
}

bool BindCaseCheck::IsSnakeCase(std::string const& in, bool caps) {
    auto checker = caps ? isLowercase : isUppercase;
    for (char ch : in) {
        // according to PEP8 function names with uppercase letters are strictly prohibited with no
        // exceptions see https://peps.python.org/pep-0008/#function-and-variable-names
        if (checker(ch)) return false;
    }
    return true;
}

std::string BindCaseCheck::ToSnakeCase(std::string const& in, bool caps) {
    std::string out;

    auto checker = caps ? isLowercase : isUppercase;
    auto transformer = caps ? toUppercase : toLowercase;

    for (std::string::size_type i = 0; i < in.length(); i++) {
        if (checker(in[i])) {
            if (i != 0 && !caps) out.push_back('_');
            out.push_back(transformer(in[i]));
        } else
            out.push_back(in[i]);
    }

    return out;
}

std::string BindCaseCheck::ToPascalCase(std::string const& in) {
    std::string out;
    bool prev_underscore = false;

    for (char a : in) {
        if (a == '_') {
            prev_underscore = true;
            continue;
        } else if (prev_underscore) {
            out.push_back(toUppercase(a));
            prev_underscore = false;
        } else {
            out.push_back(a);
        }
    }

    if (out.size() >= 1) out[0] = toUppercase(out[0]);

    return out;
}

bool BindCaseCheck::IsPascalCase(std::string const& in) {
    if (in.size() == 0) return true;
    if (!isUppercase(in[0])) return false;

    for (char a : in) {
        if (a == '_') return false;
    }

    return true;
}
};  // namespace clang::tidy::misc