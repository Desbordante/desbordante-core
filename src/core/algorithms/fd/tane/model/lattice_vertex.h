#pragma once

#include <list>
#include <utility>
#include <variant>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/model/index.h"
#include "core/model/table/position_list_index.h"
#include "core/model/table/position_list_index_with_singletons.h"

namespace model {

class LatticeVertex {
private:
    using PLIPtr = std::variant<std::unique_ptr<PositionListIndex>, PositionListIndex const*,
                                std::unique_ptr<PLIWS>, PLIWS const*>;

    boost::dynamic_bitset<> vertical_;
    // holds either an owned PLI (unique_ptr) or a non-owned one (const*)
    PLIPtr position_list_index_;
    boost::dynamic_bitset<> rhs_candidates_;
    bool is_key_candidate_;
    std::vector<LatticeVertex const*> parents_;
    bool is_invalid_;

public:
    explicit LatticeVertex(boost::dynamic_bitset<> vertical, boost::dynamic_bitset<> rhs_candidates,
                           bool is_key_candidate,
                           bool is_invalid, std::vector<LatticeVertex const*> parents = {},
                           PLIPtr pli_ptr = (PositionListIndex const*)nullptr)
        : vertical_(std::move(vertical)),
          position_list_index_(std::move(pli_ptr)),
          rhs_candidates_(std::move(rhs_candidates)),
          is_key_candidate_(is_key_candidate),
          parents_(std::move(parents)),
          is_invalid_(is_invalid) {}

    std::vector<LatticeVertex const*>& GetParents() {
        return parents_;
    }

    boost::dynamic_bitset<> const& GetVertical() const {
        return vertical_;
    }

    boost::dynamic_bitset<>& GetRhsCandidates() {
        return rhs_candidates_;
    }

    boost::dynamic_bitset<> const& GetConstRhsCandidates() const {
        return rhs_candidates_;
    }

    void SetAllRhsCandidates() {
        rhs_candidates_.set();
    }

    bool ComesBeforeAndSharePrefixWith(LatticeVertex const& that) const;

    bool GetIsKeyCandidate() const {
        return is_key_candidate_;
    }

    void SetKeyCandidate(bool m_is_key_candidate) {
        is_key_candidate_ = m_is_key_candidate;
    }

    bool GetIsInvalid() const {
        return is_invalid_;
    }

    void SetInvalid(bool m_is_invalid) {
        is_invalid_ = m_is_invalid;
    }

    PositionListIndex const* GetPositionListIndex() const;

    PLIWithSingletons const* GetPositionListIndexWithSingletons() const;

    void SetPositionListIndex(PositionListIndex const* position_list_index) {
        position_list_index_ = position_list_index;
    }

    void AcquirePositionListIndex(std::unique_ptr<PositionListIndex> position_list_index) {
        position_list_index_ = std::move(position_list_index);
    }

    void SetPLIWithSingletons(PLIWithSingletons const* position_list_index) {
        position_list_index_ = position_list_index;
    }

    void AcquirePLIWithSingletons(std::unique_ptr<PLIWithSingletons> position_list_index) {
        position_list_index_ = std::move(position_list_index);
    }

    bool operator>(LatticeVertex const& that) const;

    static bool Comparator(LatticeVertex* v1, LatticeVertex* v2) {
        return *v2 > *v1;
    }
};

}  // namespace model
