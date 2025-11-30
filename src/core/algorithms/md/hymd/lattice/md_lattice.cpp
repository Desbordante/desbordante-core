#include "core/algorithms/md/hymd/lattice/md_lattice.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <numeric>
#include <type_traits>

#include "core/algorithms/md/hymd/lattice/md_specialization.h"
#include "core/algorithms/md/hymd/lattice/multi_md_specialization.h"
#include "core/algorithms/md/hymd/lattice/rhs.h"
#include "core/algorithms/md/hymd/lattice/spec_generalization_checker.h"
#include "core/algorithms/md/hymd/lattice/total_generalization_checker.h"
#include "core/algorithms/md/hymd/lowest_cc_value_id.h"
#include "core/algorithms/md/hymd/utility/index_range.h"
#include "core/algorithms/md/hymd/utility/zip.h"
#include "core/util/desbordante_assume.h"
#include "core/util/erase_if_replace.h"

namespace {
using model::Index;
using namespace algos::hymd;
using namespace algos::hymd::lattice;
template <typename MdInfoType>
using MdGenChecker = TotalGeneralizationChecker<MdNode, MdInfoType>;
template <typename MdInfoType>
using MdSpecGenChecker = SpecGeneralizationChecker<MdNode, MdInfoType>;

template <typename MdInfoType, typename FGetLhsCCVId, typename FGetNonLhsCCVId>
class Specializer {
    using SupportCheckMethod = bool (Specializer::*)();
    using UpdateMaxLevelMethod = void (Specializer::*)();
    using SpecGenCheckerType = MdSpecGenChecker<MdInfoType>;
    using GenCheckMethod = bool (SpecGenCheckerType::*)(MdNode const&, MdLhs::iterator, Index);
    using MdCCVIdChildMap = MdNode::OrderedCCVIdChildMap;
    static constexpr bool kSpecializingSingle = std::is_same_v<MdInfoType, MdSpecialization>;
    using RhsType =
            std::conditional_t<kSpecializingSingle, MdElement, utility::ExclusionList<MdElement>&>;

    class GeneralizationHelper {
        using Unspecialized = typename MdInfoType::Unspecialized;
        MdNode* node_;
        MdGenChecker<Unspecialized>& gen_checker_;

    public:
        MdNode& CurNode() noexcept {
            return *node_;
        }

        auto& Children() noexcept {
            return node_->children;
        }

        auto& GetTotalChecker() noexcept {
            return gen_checker_;
        }

        bool SetAndCheck(MdNode* node_ptr) noexcept {
            node_ = node_ptr;
            if (!node_) return true;
            return gen_checker_.CheckNode(*node_);
        }

        void SetRhsOnCurrent() noexcept {
            node_->SetRhs(gen_checker_.GetUnspecialized().GetRhs());
        }

        GeneralizationHelper(MdNode& node, MdGenChecker<Unspecialized>& gen_checker) noexcept
            : node_(&node), gen_checker_(gen_checker) {}
    };

    using HandleTailMethod = void (Specializer::*)(GeneralizationHelper&);

    template <GenCheckMethod CheckGeneralization, HandleTailMethod HandleTail>
    void AddSpecializationSingleRhs() {
        auto const& [index, ccv_id] = GetRhs();
        if (index == lhs_spec_index_) {
            if (prune_nondisjoint_) return;

            ColumnClassifierValueId const ccv_id_triviality_bound =
                    lhs_ccv_id_info_[lhs_spec_index_].lhs_to_rhs_map[GetNewCCVId()];
            if (ccv_id <= ccv_id_triviality_bound) return;
        }

        AddIfMinimal<CheckGeneralization, HandleTail>();
    }

    template <GenCheckMethod CheckGeneralization, HandleTailMethod HandleTail>
    void AddSpecializationMultiRhs() {
        GetRhs().Reset();

        Rhss const& rhss = GetRhs().GetElements();
        DESBORDANTE_ASSUME(rhss.size() >= 2);
        DESBORDANTE_ASSUME(!rhss.empty());
        for (auto rhs_it = rhss.begin(), end = rhss.end(); rhs_it != end; ++rhs_it) {
            auto const& [index, ccv_id] = *rhs_it;
            if (index == lhs_spec_index_) {
                Index const rhs_index = std::distance(rhss.begin(), rhs_it);
                if (prune_nondisjoint_) {
                    GetRhs().GetEnabled().set(rhs_index, false);
                } else {
                    ColumnClassifierValueId const ccv_id_triviality_bound =
                            lhs_ccv_id_info_[lhs_spec_index_].lhs_to_rhs_map[GetNewCCVId()];
                    if (ccv_id <= ccv_id_triviality_bound)
                        GetRhs().GetEnabled().set(rhs_index, false);
                }
                break;
            }
        }
        // Only one bit can be unset, but there are more than one RHSs.
        assert(!GetRhs().GetEnabled().none());

        AddIfMinimal<CheckGeneralization, HandleTail>();
    }

    template <GenCheckMethod CheckGeneralization, HandleTailMethod HandleTail>
    void AddSpecialization() {
        if constexpr (kSpecializingSingle) {
            AddSpecializationSingleRhs<CheckGeneralization, HandleTail>();
        } else {
            AddSpecializationMultiRhs<CheckGeneralization, HandleTail>();
        }
    }

    MdNode* TryGetNextNode(GeneralizationHelper& helper, Index const next_node_offset,
                           auto new_minimal_action, ColumnClassifierValueId const next_lhs_ccv_id,
                           MdLhs::iterator next_node_iter, std::size_t gen_check_offset = 0) {
        MdNode& cur_node = helper.CurNode();
        MdCCVIdChildMap& child_map = cur_node.children[next_node_offset];
        if (child_map.empty()) [[unlikely]] {
            MdNode& new_node =
                    child_map
                            .try_emplace(next_lhs_ccv_id, column_matches_size_,
                                         cur_node.GetNextNodeChildArraySize(next_node_offset))
                            .first->second;
            new_minimal_action(new_node);
            return nullptr;
        }
        return TryGetNextNodeChildMap(
                child_map, helper, next_node_offset, new_minimal_action, next_lhs_ccv_id,
                next_node_iter, [](MdCCVIdChildMap& child_map) { return child_map.begin(); },
                gen_check_offset);
    }

    MdNode* TryGetNextNodeChildMap(MdCCVIdChildMap& child_map, GeneralizationHelper& helper,
                                   Index next_node_offset, auto new_minimal_action,
                                   ColumnClassifierValueId const next_lhs_ccv_id,
                                   MdLhs::iterator next_node_iter, auto get_child_map_iter,
                                   std::size_t gen_check_offset = 0) {
        MdNode& cur_node = helper.CurNode();
        auto it = get_child_map_iter(child_map);
        auto& total_checker = helper.GetTotalChecker();
        for (auto end_it = child_map.end(); it != end_it; ++it) {
            auto const& [generalization_ccv_id, next_node] = *it;
            if (generalization_ccv_id > next_lhs_ccv_id) break;
            if (generalization_ccv_id == next_lhs_ccv_id) return &it->second;
            if (total_checker.HasGeneralization(next_node, next_node_iter, gen_check_offset))
                return nullptr;
        }
        using std::forward_as_tuple;
        MdNode& new_node =
                child_map
                        .emplace_hint(it, std::piecewise_construct,
                                      forward_as_tuple(next_lhs_ccv_id),
                                      forward_as_tuple(
                                              column_matches_size_,
                                              cur_node.GetNextNodeChildArraySize(next_node_offset)))
                        ->second;
        new_minimal_action(new_node);
        return nullptr;
    }

    template <GenCheckMethod CheckGeneralization, HandleTailMethod HandleTail>
    void AddIfMinimal() {
        MdSpecGenChecker<MdInfoType> gen_checker{current_specialization_};
        auto& total_checker = gen_checker.GetTotalChecker();
        auto helper = GeneralizationHelper(md_root_, total_checker);

        MdLhs::iterator next_lhs_iter = GetLhs().begin();
        while (next_lhs_iter != GetSpecLhsIter()) {
            auto const& [next_node_offset, next_lhs_ccv_id] = *next_lhs_iter;
            ++next_lhs_iter;
            bool const next_has_generalization = (gen_checker.*CheckGeneralization)(
                    helper.CurNode(), next_lhs_iter, next_node_offset + 1);
            if (next_has_generalization) return;

            MdCCVIdChildMap& child_map = helper.Children()[next_node_offset];
            DESBORDANTE_ASSUME(!child_map.empty());
            assert(child_map.find(next_lhs_ccv_id) != child_map.end());
            auto it = child_map.begin();
            for (; it->first != next_lhs_ccv_id; ++it) {
                bool const child_has_generalization =
                        (gen_checker.*CheckGeneralization)(it->second, next_lhs_iter, 0);
                if (child_has_generalization) return;
            }
            helper.SetAndCheck(&it->second);
        }
        (this->*HandleTail)(helper);
    }

    bool SpecCCVDoesNotExist() {
        std::vector<ColumnClassifierValueId> const& lhs_ccv_ids =
                lhs_ccv_id_info_[lhs_spec_index_].lhs_to_rhs_map;
        // TODO: enforce this with a special class (basically a vector that guarantees this
        // condition).
        DESBORDANTE_ASSUME(!lhs_ccv_ids.empty());

        DESBORDANTE_ASSUME(GetNewCCVId() <= lhs_ccv_ids.size());
        return GetNewCCVId() == lhs_ccv_ids.size();
    }

    template <SupportCheckMethod CheckSupport, GenCheckMethod CheckGeneralization,
              HandleTailMethod HandleTail>
    void SpecializeElement() {
        if (SpecCCVDoesNotExist()) return;
        bool const is_unsupported = (this->*CheckSupport)();
        if (is_unsupported) return;
        AddSpecialization<CheckGeneralization, HandleTail>();
    }

    bool IsUnsupportedReplace() {
        return SpecGeneralizationChecker<SupportNode>{GetLhsSpecialization()}
                .HasGeneralizationReplace(support_root_);
    }

    bool IsUnsupportedNonReplace() {
        return SpecGeneralizationChecker<SupportNode>{GetLhsSpecialization()}
                .HasGeneralizationNonReplace(support_root_);
    }

    void UpdateMaxLevel(std::size_t const non_spec_level) {
        DESBORDANTE_ASSUME(get_element_level_);
        std::size_t const new_level =
                non_spec_level + get_element_level_(GetNewCCVId(), lhs_spec_index_);
        if (new_level > max_level_) max_level_ = new_level;
    }

    void UpdateMaxLevelAdd() {
        UpdateMaxLevel(base_level_);
    }

    void UpdateMaxLevelReplace() {
        DESBORDANTE_ASSUME(GetSpecLhsIter() != GetEndLhsIter());
        Index const lhs_node_index = std::distance(GetLhs().begin(), GetSpecLhsIter());
        UpdateMaxLevel(base_level_ - element_levels_[lhs_node_index]);
    }

    template <UpdateMaxLevelMethod UpdateMaxLevel>
    void AddNewMinimal(MdNode& cur_node, MdLhs::iterator cur_node_iter) {
        assert(cur_node.rhs.IsEmpty());
        DESBORDANTE_ASSUME(cur_node_iter >= GetSpecLhsIter());
        auto set_rhs = [&](MdNode* node) { node->SetRhs(GetRhs()); };
        AddUnchecked(
                &cur_node, GetLhs(), cur_node_iter, set_rhs,
                [&](MdNode* node, Index next_node_offset, ColumnClassifierValueId next_ccv_id) {
                    return node->AddOneUnchecked(next_node_offset, next_ccv_id,
                                                 column_matches_size_);
                });
        if (get_element_level_) (this->*UpdateMaxLevel)();
    }

    template <UpdateMaxLevelMethod UpdateMaxLevel>
    void WalkToTail(GeneralizationHelper& helper, MdLhs::iterator next_lhs_iter) {
        auto& total_checker = helper.GetTotalChecker();
        while (next_lhs_iter != GetEndLhsIter()) {
            auto const& [next_node_offset, next_lhs_ccv_id] = *next_lhs_iter;
            ++next_lhs_iter;
            auto add_normal = [&](MdNode& node) {
                AddNewMinimal<UpdateMaxLevel>(node, next_lhs_iter);
            };
            if (total_checker.HasGeneralizationInChildren(helper.CurNode(), next_lhs_iter,
                                                          next_node_offset + 1))
                return;
            if (helper.SetAndCheck(TryGetNextNode(helper, next_node_offset, add_normal,
                                                  next_lhs_ccv_id, next_lhs_iter)))
                return;
        }
        helper.SetRhsOnCurrent();
    }

    void AddIfMinimalAppend(GeneralizationHelper& helper) {
        DESBORDANTE_ASSUME(GetSpecLhsIter() == GetEndLhsIter());
        auto const& [end_node_offset, new_node_ccv_id] = GetNewChildNode();

        auto set_and_update_max_level = [this](MdNode& node) {
            assert(node.IsEmpty());
            node.SetRhs(GetRhs());
            UpdateMaxLevelAdd();
        };
        if (helper.SetAndCheck(TryGetNextNode(helper, end_node_offset, set_and_update_max_level,
                                              new_node_ccv_id, GetEndLhsIter())))
            return;
        helper.SetRhsOnCurrent();
    }

    void AddIfMinimalReplace(GeneralizationHelper& helper) {
        DESBORDANTE_ASSUME(GetSpecLhsIter() != GetEndLhsIter());
        auto const& [new_node_offset, new_node_ccv_id] = GetNewChildNode();
        auto const& [old_node_offset, old_node_ccv_id] = *GetSpecLhsIter();
        DESBORDANTE_ASSUME(new_node_offset == old_node_offset);
        DESBORDANTE_ASSUME(old_node_ccv_id < new_node_ccv_id);

        DESBORDANTE_ASSUME(!helper.Children()[old_node_offset].empty());
        // clang-tidy false positive bypass.
        auto get_higher = [old_node_ccv_id = old_node_ccv_id](MdCCVIdChildMap& child_map) {
            return child_map.upper_bound(old_node_ccv_id);
        };
        MdLhs::iterator spec_iter = std::next(GetSpecLhsIter());
        auto add_normal = [&](MdNode& node) {
            AddNewMinimal<&Specializer::UpdateMaxLevelReplace>(node, spec_iter);
        };
        if (helper.SetAndCheck(TryGetNextNodeChildMap(helper.Children()[old_node_offset], helper,
                                                      new_node_offset, add_normal, new_node_ccv_id,
                                                      spec_iter, get_higher)))
            return;
        WalkToTail<&Specializer::UpdateMaxLevelReplace>(helper, spec_iter);
    }

    void AddIfMinimalInsert(GeneralizationHelper& helper) {
        DESBORDANTE_ASSUME(GetSpecLhsIter() != GetEndLhsIter());
        auto const& [new_next_node_offset, new_next_node_ccv_id] = GetNewChildNode();
        auto const& [old_lhs_next_node_offset, old_lhs_next_node_ccv_id] = *GetSpecLhsIter();
        DESBORDANTE_ASSUME(old_lhs_next_node_offset > new_next_node_offset);

        // Number that, when added to the old offset of the next node, gives its new offset.
        std::size_t const old_offset_addend = -(new_next_node_offset + 1);
        std::size_t const old_lhs_next_node_new_offset =
                old_lhs_next_node_offset + old_offset_addend;

        auto add_old_nodes_with_correction = [&](MdNode& node) {
            AddNewMinimal<&Specializer::UpdateMaxLevelAdd>(
                    *node.AddOneUnchecked(old_lhs_next_node_new_offset, old_lhs_next_node_ccv_id,
                                          column_matches_size_),
                    std::next(GetSpecLhsIter()));
        };

        auto& total_checker = helper.GetTotalChecker();
        if (helper.SetAndCheck(TryGetNextNode(helper, new_next_node_offset,
                                              add_old_nodes_with_correction, new_next_node_ccv_id,
                                              GetSpecLhsIter(), old_offset_addend)))
            return;
        if (total_checker.HasGeneralizationInChildren(helper.CurNode(), GetSpecLhsIter(),
                                                      old_offset_addend))
            return;
        MdLhs::iterator spec_iter = std::next(GetSpecLhsIter());
        auto add_normal = [&](MdNode& node) {
            AddNewMinimal<&Specializer::UpdateMaxLevelAdd>(node, spec_iter);
        };
        if (helper.SetAndCheck(TryGetNextNode(helper, old_lhs_next_node_new_offset, add_normal,
                                              old_lhs_next_node_ccv_id, spec_iter)))
            return;
        WalkToTail<&Specializer::UpdateMaxLevelAdd>(helper, spec_iter);
    }

    void SetLhsCCVID() {
        GetNewChildNode().ccv_id = get_lhs_ccv_id_(lhs_spec_index_, GetSpecLhsIter()->ccv_id) + 1;
    }

    void SetNonLhsCCVID() {
        GetNewChildNode().ccv_id = get_nonlhs_ccv_id_(lhs_spec_index_) + 1;
    }

    void SpecializeReplaceOnly() {
        for (; GetSpecLhsIter() != GetEndLhsIter(); AdvanceLhsSpecIter(), ++lhs_spec_index_) {
            Index const next_node_offset = GetSpecLhsIter()->offset;
            lhs_spec_index_ += next_node_offset;
            GetNewChildNode().offset = next_node_offset;
            SetLhsCCVID();
            SpecializeElement<&Specializer::IsUnsupportedReplace,
                              &SpecGenCheckerType::HasGeneralizationInChildrenReplace,
                              &Specializer::AddIfMinimalReplace>();
        }
    }

    void SpecializeFull() {
        Index& spec_child_index = GetNewChildNode().offset;
        for (; GetSpecLhsIter() != GetEndLhsIter(); AdvanceLhsSpecIter(), ++lhs_spec_index_) {
            Index const next_node_offset = GetSpecLhsIter()->offset;
            for (spec_child_index = 0; spec_child_index != next_node_offset; ++spec_child_index) {
                SetNonLhsCCVID();
                SpecializeElement<&Specializer::IsUnsupportedNonReplace,
                                  &SpecGenCheckerType::HasGeneralizationInChildrenNonReplace,
                                  &Specializer::AddIfMinimalInsert>();
                ++lhs_spec_index_;
            }
            SetLhsCCVID();
            SpecializeElement<&Specializer::IsUnsupportedReplace,
                              &SpecGenCheckerType::HasGeneralizationInChildrenReplace,
                              &Specializer::AddIfMinimalReplace>();
        }
        for (spec_child_index = 0; lhs_spec_index_ != column_matches_size_; ++spec_child_index) {
            SetNonLhsCCVID();
            SpecializeElement<&Specializer::IsUnsupportedNonReplace,
                              &SpecGenCheckerType::HasGeneralizationInChildrenNonReplace,
                              &Specializer::AddIfMinimalAppend>();
            ++lhs_spec_index_;
        }
    }

    auto& GetRhs() noexcept {
        return current_specialization_.GetRhs();
    }

    LhsSpecialization& GetLhsSpecialization() noexcept {
        return current_specialization_.GetLhsSpecialization();
    }

    MdLhs const& GetLhs() noexcept {
        return GetLhsSpecialization().old_lhs;
    }

    MdLhs::iterator GetSpecLhsIter() noexcept {
        return GetLhsSpecialization().specialization_data.spec_before;
    }

    MdLhs::iterator GetEndLhsIter() noexcept {
        return GetLhs().end();
    }

    void AdvanceLhsSpecIter() noexcept {
        ++current_specialization_.GetLhsSpecialization().specialization_data.spec_before;
    }

    std::vector<std::size_t> GetElementLevels() {
        std::vector<std::size_t> element_levels =
                util::GetPreallocatedVector<std::size_t>(GetLhs().Cardinality());
        Index column_match_index = 0;
        for (auto const& [offset, ccv_id] : GetLhs()) {
            column_match_index += offset;
            element_levels.push_back(get_element_level_(ccv_id, column_match_index));
            ++column_match_index;
        }
        return element_levels;
    }

    ColumnClassifierValueId GetNewCCVId() {
        return GetNewChildNode().ccv_id;
    }

    LhsNode& GetNewChildNode() {
        return current_specialization_.GetLhsSpecialization().specialization_data.new_child;
    }

    MdNode& md_root_;
    SupportNode& support_root_;
    SingleLevelFunc const& get_element_level_;
    std::size_t& max_level_;
    std::size_t const cardinality_limit_;
    std::size_t const column_matches_size_;
    std::vector<LhsCCVIdsInfo> const& lhs_ccv_id_info_;
    FGetLhsCCVId get_lhs_ccv_id_;
    FGetNonLhsCCVId get_nonlhs_ccv_id_;
    bool const prune_nondisjoint_;

    MdInfoType current_specialization_;

    std::vector<std::size_t> const element_levels_ = GetElementLevels();
    std::size_t const base_level_ =
            std::accumulate(element_levels_.begin(), element_levels_.end(), 0);

    Index lhs_spec_index_ = 0;
    MdLhs::iterator const lhs_end_ = GetLhs().end();

public:
    Specializer(MdNode& md_root, SupportNode& support_root,
                SingleLevelFunc const& get_element_level, std::size_t& max_level,
                std::size_t const cardinality_limit, std::size_t const column_matches_size,
                std::vector<LhsCCVIdsInfo> const& lhs_ccv_id_info, MdLhs const& lhs,
                FGetLhsCCVId get_lhs_ccv_id, FGetNonLhsCCVId get_nonlhs_ccv_id,
                bool prune_nondisjoint, RhsType rhs)
        : md_root_(md_root),
          support_root_(support_root),
          get_element_level_(get_element_level),
          max_level_(max_level),
          cardinality_limit_(cardinality_limit),
          column_matches_size_(column_matches_size),
          lhs_ccv_id_info_(lhs_ccv_id_info),
          get_lhs_ccv_id_(std::move(get_lhs_ccv_id)),
          get_nonlhs_ccv_id_(std::move(get_nonlhs_ccv_id)),
          prune_nondisjoint_(prune_nondisjoint),
          current_specialization_(
                  {LhsSpecialization{lhs, SpecializationData{lhs.begin(), LhsNode{}}}, rhs}) {}

    void Specialize() {
        if (GetLhs().Cardinality() == cardinality_limit_) {
            SpecializeReplaceOnly();
            return;
        }
        SpecializeFull();
    }
};
}  // namespace

namespace algos::hymd::lattice {

// TODO: remove recursion
MdLattice::MdLattice(SingleLevelFunc single_level_func,
                     std::vector<LhsCCVIdsInfo> const& lhs_ccv_id_info, bool prune_nondisjoint,
                     std::size_t max_cardinality, Rhs max_rhs)
    : column_matches_size_(lhs_ccv_id_info.size()),
      md_root_(column_matches_size_, std::move(max_rhs)),
      support_root_(column_matches_size_),
      get_element_level_(std::move(single_level_func)),
      lhs_ccv_id_info_(&lhs_ccv_id_info),
      prune_nondisjoint_(prune_nondisjoint),
      cardinality_limit_(max_cardinality) {
    enabled_rhs_indices_.resize(column_matches_size_, true);
}

inline void MdLattice::Specialize(MdLhs const& lhs,
                                  PairComparisonResult const& pair_comparison_result,
                                  Rhss const& rhss) {
    auto get_pair_lhs_ccv_id = [this, pair_comparison_result](Index index, ...) {
        return (*lhs_ccv_id_info_)[index].rhs_to_lhs_map[pair_comparison_result.rhss[index]];
    };
    Specialize(lhs, rhss, get_pair_lhs_ccv_id, get_pair_lhs_ccv_id);
}

inline void MdLattice::Specialize(MdLhs const& lhs, Rhss const& rhss) {
    auto get_lowest = [](...) { return kLowestCCValueId; };
    auto get_lhs_ccv_id = [](Index, ColumnClassifierValueId ccv_id) { return ccv_id; };
    Specialize(lhs, rhss, get_lhs_ccv_id, get_lowest);
}

template <typename MdInfoType>
auto MdLattice::CreateSpecializer(MdLhs const& lhs, auto&& rhs, auto get_lhs_ccv_id,
                                  auto get_nonlhs_ccv_id) {
    return Specializer<MdInfoType, decltype(get_lhs_ccv_id), decltype(get_nonlhs_ccv_id)>(
            md_root_, support_root_, get_element_level_, max_level_, cardinality_limit_,
            column_matches_size_, *lhs_ccv_id_info_, lhs, get_lhs_ccv_id, get_nonlhs_ccv_id,
            prune_nondisjoint_, rhs);
}

void MdLattice::Specialize(MdLhs const& lhs, Rhss const& rhss, auto get_lhs_ccv_id,
                           auto get_nonlhs_ccv_id) {
    switch (rhss.size()) {
        case 0:
            break;
        case 1:
            CreateSpecializer<MdSpecialization>(lhs, rhss.front(), get_lhs_ccv_id,
                                                get_nonlhs_ccv_id)
                    .Specialize();
            break;
        default: {
            auto enabled_rhss =
                    utility::ExclusionList<MdElement>::Create(rhss, enabled_rhs_indices_);
            CreateSpecializer<MultiMdSpecialization>(lhs, enabled_rhss, get_lhs_ccv_id,
                                                     get_nonlhs_ccv_id)
                    .Specialize();
        }
    }
}

std::size_t MdLattice::MdRefiner::Refine() {
    std::size_t removed = 0;
    Rhs& rhs = node_info_.node->rhs;
    for (auto const& new_rhs : invalidated_.GetUpdateView()) {
        auto const& [rhs_index, new_ccv_id] = new_rhs;
        DESBORDANTE_ASSUME(rhs.begin[rhs_index] != kLowestCCValueId);
        rhs.Set(rhs_index, kLowestCCValueId);
        bool const trivial = new_ccv_id == kLowestCCValueId;
        if (trivial) {
            ++removed;
            continue;
        }
        bool const not_minimal = lattice_->HasGeneralization({GetLhs(), new_rhs});
        if (not_minimal) {
            ++removed;
            continue;
        }
        DESBORDANTE_ASSUME(rhs.begin[rhs_index] == kLowestCCValueId &&
                           new_ccv_id != kLowestCCValueId);
        rhs.Set(rhs_index, new_ccv_id);
    }
    lattice_->Specialize(GetLhs(), *pair_comparison_result_, invalidated_.GetInvalidated());
    if (rhs.IsEmpty() && node_info_.node->IsEmpty()) {
        lattice_->TryDeleteEmptyNode<false>(GetLhs());
    }
    return removed;
}

void MdLattice::TryAddRefiner(std::vector<MdRefiner>& found, MdNode& cur_node,
                              PairComparisonResult const& pair_comparison_result,
                              MdLhs const& cur_node_lhs) {
    Rhs& rhs = cur_node.rhs;
    utility::InvalidatedRhss invalidated;
    Index rhs_index = 0;
    Index cur_lhs_index = 0;
    auto try_push_no_match_classifier = [&]() {
        ColumnClassifierValueId pair_ccv_id = pair_comparison_result.rhss[rhs_index];
        ColumnClassifierValueId rhs_ccv_id = rhs[rhs_index];
        if (pair_ccv_id < rhs_ccv_id) {
            invalidated.PushBack({rhs_index, rhs_ccv_id}, pair_ccv_id);
        }
    };
    for (auto const& [child_index, lhs_ccv_id] : cur_node_lhs) {
        cur_lhs_index += child_index;
        for (; rhs_index != cur_lhs_index; ++rhs_index) {
            try_push_no_match_classifier();
        }
        DESBORDANTE_ASSUME(rhs_index < column_matches_size_);
        ColumnClassifierValueId const pair_ccv_id = pair_comparison_result.rhss[rhs_index];
        ColumnClassifierValueId const rhs_ccv_id = rhs[rhs_index];
        if (pair_ccv_id < rhs_ccv_id) {
            MdElement invalid{rhs_index, rhs_ccv_id};
            ColumnClassifierValueId cur_lhs_triviality_bound =
                    (*lhs_ccv_id_info_)[cur_lhs_index].lhs_to_rhs_map[lhs_ccv_id];
            if (cur_lhs_triviality_bound == pair_ccv_id) {
                invalidated.PushBack(invalid, kLowestCCValueId);
            } else {
                DESBORDANTE_ASSUME(pair_ccv_id > cur_lhs_triviality_bound);
                invalidated.PushBack(invalid, pair_ccv_id);
            }
        }
        ++rhs_index;
        ++cur_lhs_index;
    }
    for (; rhs_index != column_matches_size_; ++rhs_index) {
        try_push_no_match_classifier();
    }
    if (invalidated.IsEmpty()) return;
    found.emplace_back(this, &pair_comparison_result, MdLatticeNodeInfo{cur_node_lhs, &cur_node},
                       std::move(invalidated));
}

void MdLattice::CollectRefinersForViolated(MdNode& cur_node, std::vector<MdRefiner>& found,
                                           MdLhs& cur_node_lhs, MdLhs::iterator cur_lhs_iter,
                                           PairComparisonResult const& pair_comparison_result) {
    if (!cur_node.rhs.IsEmpty()) {
        TryAddRefiner(found, cur_node, pair_comparison_result, cur_node_lhs);
    }

    Index total_offset = 0;
    for (MdLhs::iterator const end = pair_comparison_result.maximal_matching_lhs.end();
         cur_lhs_iter != end; ++total_offset) {
        auto const& [next_node_offset, generalization_ccv_id_limit] = *cur_lhs_iter;
        ++cur_lhs_iter;
        total_offset += next_node_offset;
        ColumnClassifierValueId& cur_lhs_ccv_id = cur_node_lhs.AddNext(total_offset);
        for (auto& [generalization_ccv_id, node] : cur_node.children[total_offset]) {
            if (generalization_ccv_id > generalization_ccv_id_limit) break;
            cur_lhs_ccv_id = generalization_ccv_id;
            CollectRefinersForViolated(node, found, cur_node_lhs, cur_lhs_iter,
                                       pair_comparison_result);
        }
        cur_node_lhs.RemoveLast();
    }
}

auto MdLattice::CollectRefinersForViolated(PairComparisonResult const& pair_comparison_result)
        -> std::vector<MdRefiner> {
    std::vector<MdRefiner> found;
    MdLhs current_lhs(pair_comparison_result.maximal_matching_lhs.Cardinality());
    CollectRefinersForViolated(md_root_, found, current_lhs,
                               pair_comparison_result.maximal_matching_lhs.begin(),
                               pair_comparison_result);
    // TODO: traverse support trie simultaneously.
    util::EraseIfReplace(found, [this](MdRefiner& refiner) {
        bool const unsupported = IsUnsupported(refiner.GetLhs());
        if (unsupported) TryDeleteEmptyNode<true>(refiner.GetLhs());
        return unsupported;
    });
    return found;
}

template <bool MayNotExist>
void MdLattice::TryDeleteEmptyNode(MdLhs const& lhs) {
    std::vector<PathInfo> path_to_node = util::GetPreallocatedVector<PathInfo>(lhs.Cardinality());
    MdNode* cur_node_ptr = &md_root_;
    for (auto const& [node_offset, ccv_id] : lhs) {
        auto& map = cur_node_ptr->children[node_offset];
        auto it = map.find(ccv_id);
        if constexpr (MayNotExist) {
            if (it == map.end()) return;
        } else {
            DESBORDANTE_ASSUME(it != map.end());
        }
        path_to_node.push_back({cur_node_ptr, &map, it});
        cur_node_ptr = &it->second;
    }

    while (!path_to_node.empty()) {
        auto& [last_node, map_ptr, it] = path_to_node.back();
        map_ptr->erase(it);
        if (!map_ptr->empty()) break;
        if (!last_node->rhs.IsEmpty()) break;
        if (!last_node->IsEmpty()) break;
        path_to_node.pop_back();
    }
}

bool MdLattice::IsUnsupported(MdLhs const& lhs) const {
    return TotalGeneralizationChecker<SupportNode>{lhs}.HasGeneralization(support_root_);
}

void MdLattice::MdVerificationMessenger::MarkUnsupported() {
    // TODO: specializations can be removed from the MD lattice. If not worth it, removing just
    // this node and its children should be cheap. Though, destructors also take time.

    // This matters. Violation search can find a node with a specialized LHS but higher RHS column
    // classifier value ID, leading to extra work (though no influence on correctness, as MDs with
    // unsupported LHSs are filtered out).

    lattice_->MarkUnsupported(GetLhs());
    lattice_->TryDeleteEmptyNode<true>(GetLhs());
}

void MdLattice::MdVerificationMessenger::LowerAndSpecialize(
        utility::InvalidatedRhss const& invalidated) {
    Rhs& rhs = GetRhs();
    for (auto [rhs_index, new_ccv_id] : invalidated.GetUpdateView()) {
        DESBORDANTE_ASSUME(rhs[rhs_index] != kLowestCCValueId);
        rhs.Set(rhs_index, new_ccv_id);
    }
    lattice_->Specialize(GetLhs(), invalidated.GetInvalidated());
    if (GetRhs().IsEmpty() && GetNode().IsEmpty()) {
        lattice_->TryDeleteEmptyNode<false>(GetLhs());
    }
}

// The original paper mentions checking for the case where all decision bounds are 1.0, but if such
// a situation occurs for any one RHS, and the generalization with that RHS happens to be valid on
// the data, it would make inference from record pairs give an incorrect result, meaning the
// algorithm is incorrect. However, it is possible to stop traversing when the bound's index in the
// list of natural decision boundaries (that being column classifier value ID) is exactly one less
// than the RHS bound's index, which is done here.
void MdLattice::RaiseInterestingnessCCVIds(
        MdNode const& cur_node, MdLhs const& lhs,
        std::vector<ColumnClassifierValueId>& cur_interestingness_ccv_ids,
        MdLhs::iterator cur_lhs_iter, std::vector<Index> const& indices,
        std::vector<ColumnClassifierValueId> const& ccv_id_bounds, std::size_t& max_count) const {
    std::size_t const indices_size = indices.size();
    if (!cur_node.rhs.IsEmpty()) {
        for (Index i : utility::IndexRange(indices_size)) {
            ColumnClassifierValueId const cur_node_rhs_ccv_id = cur_node.rhs[indices[i]];
            ColumnClassifierValueId& cur_interestingness_ccv_id = cur_interestingness_ccv_ids[i];
            if (cur_node_rhs_ccv_id > cur_interestingness_ccv_id) {
                cur_interestingness_ccv_id = cur_node_rhs_ccv_id;
                if (cur_interestingness_ccv_id == ccv_id_bounds[i]) {
                    ++max_count;
                    if (max_count == indices_size) return;
                }
            }
        }
    }

    Index total_offset = 0;
    for (MdLhs::iterator const end = lhs.end(); cur_lhs_iter != end; ++total_offset) {
        auto const& [next_node_offset, generalization_ccv_id_limit] = *cur_lhs_iter;
        ++cur_lhs_iter;
        total_offset += next_node_offset;
        for (auto const& [generalization_ccv_id, node] : cur_node.children[total_offset]) {
            if (generalization_ccv_id > generalization_ccv_id_limit) break;
            RaiseInterestingnessCCVIds(node, lhs, cur_interestingness_ccv_ids, cur_lhs_iter,
                                       indices, ccv_id_bounds, max_count);
            if (max_count == indices_size) return;
        }
    }
}

std::vector<ColumnClassifierValueId> MdLattice::GetInterestingnessCCVIds(
        MdLhs const& lhs, std::vector<Index> const& indices,
        std::vector<ColumnClassifierValueId> const& ccv_id_bounds) const {
    std::vector<ColumnClassifierValueId> interestingness_ccv_ids;
    std::size_t const indices_size = indices.size();
    if (prune_nondisjoint_) {
        interestingness_ccv_ids.assign(indices_size, kLowestCCValueId);
    } else {
        interestingness_ccv_ids.reserve(indices_size);
        assert(std::is_sorted(indices.begin(), indices.end()));
        auto fill_interestingness_ccv_ids = [&]() {
            auto index_it = indices.begin(), index_end = indices.end();
            Index cur_index = 0;
            DESBORDANTE_ASSUME(!indices.empty());
            for (auto const& [child_index, lhs_ccv_id] : lhs) {
                cur_index += child_index;
                Index index;
                while ((index = *index_it) < cur_index) {
                    DESBORDANTE_ASSUME(
                            (*lhs_ccv_id_info_)[index].lhs_to_rhs_map[kLowestCCValueId] ==
                            kLowestCCValueId);
                    interestingness_ccv_ids.push_back(kLowestCCValueId);
                    if (++index_it == index_end) return;
                }
                if (cur_index == index) {
                    interestingness_ccv_ids.push_back(
                            (*lhs_ccv_id_info_)[index].lhs_to_rhs_map[lhs_ccv_id]);
                    if (++index_it == index_end) return;
                }
                ++cur_index;
            }
            while (index_it != index_end) {
                DESBORDANTE_ASSUME(
                        (*lhs_ccv_id_info_)[*index_it].lhs_to_rhs_map[kLowestCCValueId] ==
                        kLowestCCValueId);
                interestingness_ccv_ids.push_back(kLowestCCValueId);
                ++index_it;
            }
        };
        fill_interestingness_ccv_ids();
    }
    std::size_t max_count = 0;
    for (auto [interestingness_ccv_id, ccv_id_bound] :
         utility::Zip(interestingness_ccv_ids, ccv_id_bounds)) {
        if (interestingness_ccv_id == ccv_id_bound) ++max_count;
    }
    if (max_count == indices_size) return interestingness_ccv_ids;

    RaiseInterestingnessCCVIds(md_root_, lhs, interestingness_ccv_ids, lhs.begin(), indices,
                               ccv_id_bounds, max_count);
    return interestingness_ccv_ids;
}

bool MdLattice::HasGeneralization(Md const& md) const {
    return MdGenChecker<Md>{md}.HasGeneralization(md_root_);
}

void MdLattice::GetLevel(MdNode& cur_node, std::vector<MdVerificationMessenger>& collected,
                         MdLhs& cur_node_lhs, Index const cur_node_column_match_index,
                         std::size_t const level_left) {
    if (level_left == 0) {
        if (!cur_node.rhs.IsEmpty())
            collected.emplace_back(this, MdLatticeNodeInfo{cur_node_lhs, &cur_node});
        return;
    }
    auto collect = [&](MdCCVIdChildMap& child_map, Index next_node_offset) {
        Index const next_node_column_match_index = cur_node_column_match_index + next_node_offset;
        ColumnClassifierValueId& next_lhs_ccv_id = cur_node_lhs.AddNext(next_node_offset);
        for (auto& [ccv_id, node] : child_map) {
            std::size_t const element_level =
                    get_element_level_(next_node_column_match_index, ccv_id);
            if (element_level > level_left) break;
            next_lhs_ccv_id = ccv_id;
            GetLevel(node, collected, cur_node_lhs, next_node_column_match_index + 1,
                     level_left - element_level);
        }
        cur_node_lhs.RemoveLast();
    };
    cur_node.ForEachNonEmpty(collect);
}

auto MdLattice::GetLevel(std::size_t const level) -> std::vector<MdVerificationMessenger> {
    std::vector<MdVerificationMessenger> collected;
    MdLhs current_lhs(column_matches_size_);
    if (!get_element_level_) {
        GetAll(md_root_, current_lhs, [this, &collected](MdLhs& cur_node_lhs, MdNode& cur_node) {
            collected.emplace_back(this, MdLatticeNodeInfo{cur_node_lhs, &cur_node});
        });
    } else {
        GetLevel(md_root_, collected, current_lhs, 0, level);
    }
    // TODO: traverse support trie simultaneously.
    util::EraseIfReplace(collected, [this](MdVerificationMessenger& messenger) {
        bool is_unsupported = IsUnsupported(messenger.GetLhs());
        if (is_unsupported) TryDeleteEmptyNode<true>(messenger.GetLhs());
        return is_unsupported;
    });
    return collected;
}

void MdLattice::GetAll(MdNode& cur_node, MdLhs& cur_node_lhs, auto&& add_node) {
    if (!cur_node.rhs.IsEmpty()) add_node(cur_node_lhs, cur_node);

    auto collect = [&](MdCCVIdChildMap& child_map, Index next_node_offset) {
        ColumnClassifierValueId& next_lhs_ccv_id = cur_node_lhs.AddNext(next_node_offset);
        for (auto& [ccv_id, node] : child_map) {
            next_lhs_ccv_id = ccv_id;
            GetAll(node, cur_node_lhs, add_node);
        }
        cur_node_lhs.RemoveLast();
    };
    cur_node.ForEachNonEmpty(collect);
}

std::vector<MdLatticeNodeInfo> MdLattice::GetAll() {
    std::vector<MdLatticeNodeInfo> collected;
    MdLhs current_lhs(column_matches_size_);
    GetAll(md_root_, current_lhs, [&collected](MdLhs& cur_node_lhs, MdNode& cur_node) {
        collected.push_back({cur_node_lhs, &cur_node});
    });
    assert(std::ranges::none_of(collected, [this](MdLatticeNodeInfo const& node_info) {
        return IsUnsupported(node_info.lhs);
    }));
    return collected;
}

void MdLattice::MarkNewLhs(SupportNode& cur_node, MdLhs const& lhs, MdLhs::iterator cur_lhs_iter) {
    AddUnchecked(&cur_node, lhs, cur_lhs_iter, SetUnsupAction());
}

void MdLattice::MarkUnsupported(MdLhs const& lhs) {
    auto mark_new = [this](auto&&... args) { MarkNewLhs(std::forward<decltype(args)>(args)...); };
    CheckedAdd(&support_root_, lhs, lhs, mark_new, SetUnsupAction());
}

}  // namespace algos::hymd::lattice
