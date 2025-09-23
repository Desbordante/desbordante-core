#pragma once

#include <string>
#include <vector>

#include "algorithms/pac/model/default_domains/domain_type.h"
#include "algorithms/pac/pac_verifier/domain_pac_verifier/domain_pac_verifier_base.h"
#include "names.h"

namespace algos::pac_verifier {
/// @brief Adapts DomainPACVerifier options to simplify its usage in CLI
class DomainPACVerifierCLIAdapter final : public DomainPACVerifierBase {
private:
    pac::model::DomainType domain_type_ = pac::model::DomainType::ball;
    std::vector<double> leveling_coeffs_;
    // Ball options
    std::vector<std::string> center_str_;
    double radius_;
    // Parallelepiped options
    std::vector<std::string> first_str_;
    std::vector<std::string> last_str_;

    void RegisterOptions();

protected:
    virtual void ProcessPACTypeOptions() override;

public:
    DomainPACVerifierCLIAdapter() : DomainPACVerifierBase() {
        using namespace config::names;

        RegisterOptions();
        MakeOptionsAvailable({kDomainType});
    }
};
}  // namespace algos::pac_verifier
