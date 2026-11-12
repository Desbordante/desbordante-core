#include "checks/pybind/bindsnakecase.h"
#include "checks/explicitcapture.h"
#include "checks/makeauto.h"
#include "checks/prohibitat.h"
#include "checks/whilenotfor.h"
#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"

namespace clang::tidy::misc {
class DesbordanteModule : public ClangTidyModule {
public:
    void addCheckFactories(ClangTidyCheckFactories& factories) override {
        factories.registerCheck<MakeAutoCheck>("desbordante.MakeAuto");
        factories.registerCheck<WhileNotForCheck>("desbordante.WhileNotFor");
        factories.registerCheck<ExplicitLambdaCapturesCheck>("desbordante.ExplicitLambdaCaptures");
        factories.registerCheck<ProhibitAtCheck>("desbordante.ProhibitAt");
        factories.registerCheck<BindSnakeCaseCheck>("desbordante.pybind.BindSnakeCase");
    }
};

ClangTidyModuleRegistry::Add<DesbordanteModule> module(
        "Desbordante", "Contains checks used by Desbordante project");
};  // namespace clang::tidy::misc
