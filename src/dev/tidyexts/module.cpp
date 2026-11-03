#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"

#include "checks/makeauto.h"

namespace clang::tidy::misc {
class DesbordanteModule : public ClangTidyModule {
public:
    void addCheckFactories(ClangTidyCheckFactories& factories) override {
        factories.registerCheck<MakeAutoCheck>("makeauto");
    }
};

ClangTidyModuleRegistry::Add<DesbordanteModule> module(
        "Desbordante", "Contains checks used by Desbordante project");
};  // namespace clang::tidy::misc
