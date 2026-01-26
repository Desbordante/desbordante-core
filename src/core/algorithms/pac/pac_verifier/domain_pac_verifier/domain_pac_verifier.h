#pragma once

#include <vector>

#include "core/algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier_base.h"
#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"

namespace algos::pac_verifier {
/// @brief Domain Probabilistic Approximate Constraint verifier.
/// Takes pointer to IDomain as domain option.
/// This version is considered to be used in C++.
class DomainPACVerifier final : public DomainPACVerifierBase {
public:
    DomainPACVerifier() : DomainPACVerifierBase() {
        DESBORDANTE_OPTION_USING;

        RegisterOption(Option(&domain_, kDomain, kDDomain));
        MakeOptionsAvailable({kDomain});
    }
};
}  // namespace algos::pac_verifier
