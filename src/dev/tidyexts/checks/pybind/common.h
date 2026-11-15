#include <clang/AST/Type.h>

namespace clang::tidy::misc {
bool IsPybindClass(QualType const& type);
};