#include "common.h"
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Type.h>

namespace clang::tidy::misc {
bool IsPybindClass(const QualType& qt) {
    auto const* rd = qt->getAsCXXRecordDecl();
    auto const* spec = llvm::dyn_cast_or_null<clang::ClassTemplateSpecializationDecl>(rd);

    if (!spec) return false;

    auto const* tmpl = spec->getSpecializedTemplate();

    return tmpl->getQualifiedNameAsString() == "pybind11::class_";
}
};  // namespace clang::tidy::misc