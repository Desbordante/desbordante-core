/** \file
 * \brief Mind algorithm
 *
 * Mind algorithm class definition
 */
#pragma once

#include <memory>
#include <optional>

#include "algorithms/ind/ind_algorithm.h"
#include "config/error/type.h"
#include "config/max_arity/type.h"
#include "raw_ind.h"

namespace algos {

///
/// \brief in-memory nary approximate inclusion dependency mining algorithm
///
class Mind final : public INDAlgorithm {
public:
    /// timing information for algorithm stages
    struct StageTimings {
        size_t load;          /**< time taken for the data loading */
        size_t compute_uinds; /**< time taken for the unary inds computing */
        size_t compute_ninds; /**< time taken for the n-ary inds computing */
    };

private:
    using RawIND = mind::RawIND;

    /* configuration stage fields */
    config::ErrorType max_ind_error_ = 0;
    config::MaxArityType max_arity_;

    /* execution stage fields */
    std::unique_ptr<INDAlgorithm> auind_algo_; /*< algorithm for mining unary approximate INDs*/
    StageTimings timings_{};                   /*< timings info */

    void MakeLoadOptsAvailable();
    void MakeExecuteOptsAvailable() override;
    void AddSpecificNeededOptions(
            std::unordered_set<std::string_view>& previous_options) const override;
    bool SetExternalOption(std::string_view option_name, boost::any const& value) override;
    std::type_index GetExternalTypeIndex(std::string_view option_name) const override;

    bool ExternalOptionIsRequired(std::string_view option_name) const override;
    void LoadINDAlgorithmDataInternal() override;

    ///
    /// Test a given IND candidate to determine if it should be registered.
    ///
    /// \return `std::nullopt` if the candidate should not be registered,
    ///         otherwise return the error threshold at which AIND holds.
    ///
    std::optional<config::ErrorType> TestCandidate(RawIND const& raw_ind);

    void MineUnaryINDs();
    void MineNaryINDs();
    unsigned long long ExecuteInternal() override;
    void ResetINDAlgorithmState() override;

public:
    explicit Mind();

    /// get information about stage timings
    StageTimings const& GetStageTimings() const noexcept {
        return timings_;
    }
};

}  // namespace algos
