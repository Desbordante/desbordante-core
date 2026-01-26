#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier_cli_adapter.h"

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/default_domains/ball.h"
#include "core/algorithms/pac/model/default_domains/domain_type.h"
#include "core/algorithms/pac/model/default_domains/parallelepiped.h"
#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier_base.h"
#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "core/config/option.h"
#include "core/config/option_using.h"

namespace algos::pac_verifier {
using namespace pac::model;

void DomainPACVerifierCLIAdapter::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    using namespace std::string_literals;

    auto make_opt_condition = [](DomainType needed_type,
                                 std::vector<std::string_view>&& options_to_activate) {
        return std::make_pair(
                [needed_type](DomainType const& d_type) { return d_type == needed_type; },
                std::move(options_to_activate));
    };

    RegisterOption(Option(&domain_type_, kDomainType, kDDomainType)
                           .SetConditionalOpts({
                                   make_opt_condition(DomainType::ball,
                                                      {kLevelingCoeffs, kCenter, kRadius}),
                                   make_opt_condition(DomainType::parallelepiped,
                                                      {kLevelingCoeffs, kFirst, kLast}),
                           })
                           .SetValueCheck([](DomainType const d_type) {
                               if (d_type == +DomainType::custom_domain) {
                                   throw config::ConfigurationError(
                                           "Custom domain is not supported in CLI. Consider using "
                                           "Python interface or C++ "
                                           "library.");
                               }
                           }));

    // These options will be made available when domain type is selected
    RegisterOption(
            Option(&leveling_coeffs_, kLevelingCoeffs, kDLevelingCoeffs, std::vector<double>{}));
    RegisterOption(Option(&center_str_, kCenter, kDCenter));
    RegisterOption(Option(&radius_, kRadius, kDPACRadius));
    RegisterOption(Option(&first_str_, kFirst, kDFirst));
    RegisterOption(Option(&last_str_, kLast, kDLast));
}

void DomainPACVerifierCLIAdapter::ProcessPACTypeOptions() {
    using namespace pac::model;

    switch (domain_type_) {
        case DomainType::ball:
            domain_ = std::make_shared<Ball>(std::move(center_str_), radius_,
                                             std::move(leveling_coeffs_));
            break;
        case DomainType::parallelepiped:
            domain_ = std::make_shared<Parallelepiped>(std::move(first_str_), std::move(last_str_),
                                                       std::move(leveling_coeffs_));
            break;
        case DomainType::custom_domain:
            __builtin_unreachable();
            break;
    }

    algos::pac_verifier::DomainPACVerifierBase::ProcessPACTypeOptions();
}
}  // namespace algos::pac_verifier
