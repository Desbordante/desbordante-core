#pragma once

#include <clang/AST/Type.h>
#include <optional>
#include <string>

#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy::misc {

// Prohibits the use of .at method on STL containers
class ProhibitAtCheck : public ClangTidyCheck {
public:
    ProhibitAtCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {}

    void registerMatchers(ast_matchers::MatchFinder* Finder) override;

    void check(ast_matchers::MatchFinder::MatchResult const& Result) override;

private:
    enum class ContainerKind : int { kMapLike, kVectorLike };

    std::string GenerateFixText(ContainerKind kind, std::string const& idx_var, bool is_arrow);
    std::optional<ContainerKind> ResolveContainerKind(QualType const& type);
};

}  // namespace clang::tidy::misc