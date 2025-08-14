#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lattice.h"

#include <numeric>

#include "algorithms/mde/hymde/cover_calculation/lattice/multi_mde.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/multi_mde_specialization.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/spec_generalization_checker.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/total_generalization_checker.h"
#include "algorithms/mde/hymde/utility/index_range.h"
#include "model/index.h"
#include "util/desbordante_assume.h"
#include "util/erase_if_replace.h"

namespace {
using model::Index;
using namespace algos::hymde;
using namespace algos::hymde::cover_calculation;
using namespace algos::hymde::cover_calculation::lattice;
template <typename MdeInfoType>
using MdGenChecker = TotalGeneralizationChecker<MdeNode, MdeInfoType>;
template <typename MdeInfoType>
using MdSpecGenChecker = SpecGeneralizationChecker<MdeNode, MdeInfoType>;

template <typename MdeInfoType, typename FGetLhsRCVId, typename FGetNonLhsRCVId>
class Specializer {
    using SupportCheckMethod = bool (Specializer::*)();
    using UpdateMaxLevelMethod = void (Specializer::*)();
    using SpecGenCheckerType = MdSpecGenChecker<MdeInfoType>;
    using GenCheckMethod = bool (SpecGenCheckerType::*)(MdeNode const&, PathToNode::iterator,
                                                        Index);
    using MdeRCVIdChildMap = MdeNode::OrderedRCVIdChildMap;
    static constexpr bool kSpecializingSingle = std::is_same_v<MdeInfoType, MdeSpecialization>;
    using RhsType = std::conditional_t<kSpecializingSingle, MdeElement,
                                       utility::ExclusionList<MdeElement>&>;

    class GeneralizationHelper {
        using Unspecialized = typename MdeInfoType::Unspecialized;
        MdeNode* node_;
        MdGenChecker<Unspecialized>& gen_checker_;

    public:
        MdeNode& CurNode() noexcept {
            return *node_;
        }

        auto& Children() noexcept {
            return node_->children;
        }

        auto& GetTotalChecker() noexcept {
            return gen_checker_;
        }

        bool SetAndCheck(MdeNode* node_ptr) noexcept {
            node_ = node_ptr;
            if (!node_) return true;
            return gen_checker_.CheckNode(*node_);
        }

        void SetRhsOnCurrent() noexcept {
            node_->SetRhs(gen_checker_.GetUnspecialized().GetRhs());
        }

        GeneralizationHelper(MdeNode& node, MdGenChecker<Unspecialized>& gen_checker) noexcept
            : node_(&node), gen_checker_(gen_checker) {}
    };

    using HandleTailMethod = void (Specializer::*)(GeneralizationHelper&);

    template <GenCheckMethod CheckGeneralization, HandleTailMethod HandleTail>
    void AddSpecializationSingleRhs() {
        auto const& [index, rcv_id] = GetRhs();
        if (index == lhs_spec_index_) {
            if (prune_nondisjoint_) return;

            RecordClassifierValueId const rcv_id_triviality_bound =
                    rcv_id_lr_maps_[lhs_spec_index_].lhs_to_rhs_map[GetNewRCVId()];
            if (rcv_id <= rcv_id_triviality_bound) return;
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
            auto const& [index, rcv_id] = *rhs_it;
            if (index == lhs_spec_index_) {
                Index const rhs_index = std::distance(rhss.begin(), rhs_it);
                if (prune_nondisjoint_) {
                    GetRhs().GetEnabled().set(rhs_index, false);
                } else {
                    RecordClassifierValueId const rcv_id_triviality_bound =
                            rcv_id_lr_maps_[lhs_spec_index_].lhs_to_rhs_map[GetNewRCVId()];
                    if (rcv_id <= rcv_id_triviality_bound)
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

    MdeNode* TryGetNextNode(GeneralizationHelper& helper, Index const next_node_offset,
                            auto new_minimal_action, RecordClassifierValueId const next_lhs_rcv_id,
                            PathToNode::iterator next_node_iter, std::size_t gen_check_offset = 0) {
        MdeNode& cur_node = helper.CurNode();
        MdeRCVIdChildMap& child_map = cur_node.children[next_node_offset];
        if (child_map.empty()) [[unlikely]] {
            MdeNode& new_node =
                    child_map
                            .try_emplace(next_lhs_rcv_id, record_matches_size_,
                                         cur_node.GetNextNodeChildArraySize(next_node_offset))
                            .first->second;
            new_minimal_action(new_node);
            return nullptr;
        }
        return TryGetNextNodeChildMap(
                child_map, helper, next_node_offset, new_minimal_action, next_lhs_rcv_id,
                next_node_iter, [](MdeRCVIdChildMap& child_map) { return child_map.begin(); },
                gen_check_offset);
    }

    MdeNode* TryGetNextNodeChildMap(MdeRCVIdChildMap& child_map, GeneralizationHelper& helper,
                                    Index next_node_offset, auto new_minimal_action,
                                    RecordClassifierValueId const next_lhs_rcv_id,
                                    PathToNode::iterator next_node_iter, auto get_child_map_iter,
                                    std::size_t gen_check_offset = 0) {
        MdeNode& cur_node = helper.CurNode();
        auto it = get_child_map_iter(child_map);
        auto& total_checker = helper.GetTotalChecker();
        for (auto end_it = child_map.end(); it != end_it; ++it) {
            auto const& [generalization_rcv_id, next_node] = *it;
            if (generalization_rcv_id > next_lhs_rcv_id) break;
            if (generalization_rcv_id == next_lhs_rcv_id) return &it->second;
            if (total_checker.HasGeneralization(next_node, next_node_iter, gen_check_offset))
                return nullptr;
        }
        using std::forward_as_tuple;
        MdeNode& new_node =
                child_map
                        .emplace_hint(it, std::piecewise_construct,
                                      forward_as_tuple(next_lhs_rcv_id),
                                      forward_as_tuple(
                                              record_matches_size_,
                                              cur_node.GetNextNodeChildArraySize(next_node_offset)))
                        ->second;
        new_minimal_action(new_node);
        return nullptr;
    }

    template <GenCheckMethod CheckGeneralization, HandleTailMethod HandleTail>
    void AddIfMinimal() {
        MdSpecGenChecker<MdeInfoType> gen_checker{current_specialization_};
        auto& total_checker = gen_checker.GetTotalChecker();
        auto helper = GeneralizationHelper(mde_root_, total_checker);

        PathToNode::iterator next_lhs_iter = GetLhs().begin();
        while (next_lhs_iter != GetSpecLhsIter()) {
            auto const& [next_node_offset, next_lhs_rcv_id] = *next_lhs_iter;
            ++next_lhs_iter;
            bool const next_has_generalization = (gen_checker.*CheckGeneralization)(
                    helper.CurNode(), next_lhs_iter, next_node_offset + 1);
            if (next_has_generalization) return;

            MdeRCVIdChildMap& child_map = helper.Children()[next_node_offset];
            DESBORDANTE_ASSUME(!child_map.empty());
            assert(child_map.find(next_lhs_rcv_id) != child_map.end());
            auto it = child_map.begin();
            for (; it->first != next_lhs_rcv_id; ++it) {
                bool const child_has_generalization =
                        (gen_checker.*CheckGeneralization)(it->second, next_lhs_iter, 0);
                if (child_has_generalization) return;
            }
            helper.SetAndCheck(&it->second);
        }
        (this->*HandleTail)(helper);
    }

    bool SpecRCVDoesNotExist() {
        std::vector<RecordClassifierValueId> const& lhs_rcv_ids =
                rcv_id_lr_maps_[lhs_spec_index_].lhs_to_rhs_map;
        // TODO: enforce this with a special class (basically a vector that guarantees this
        // condition).
        DESBORDANTE_ASSUME(!lhs_rcv_ids.empty());

        DESBORDANTE_ASSUME(GetNewRCVId() <= lhs_rcv_ids.size());
        return GetNewRCVId() == lhs_rcv_ids.size();
    }

    template <SupportCheckMethod CheckSupport, GenCheckMethod CheckGeneralization,
              HandleTailMethod HandleTail>
    void SpecializeElement() {
        if (SpecRCVDoesNotExist()) return;
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
                non_spec_level + get_element_level_(GetNewRCVId(), lhs_spec_index_);
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
    void AddNewMinimal(MdeNode& cur_node, PathToNode::iterator cur_node_iter) {
        assert(cur_node.rhs.IsEmpty());
        DESBORDANTE_ASSUME(cur_node_iter >= GetSpecLhsIter());
        auto set_rhs = [&](MdeNode* node) { node->SetRhs(GetRhs()); };
        AddUnchecked(
                &cur_node, GetLhs(), cur_node_iter, set_rhs,
                [&](MdeNode* node, Index next_node_offset, RecordClassifierValueId next_rcv_id) {
                    return node->AddOneUnchecked(next_node_offset, next_rcv_id,
                                                 record_matches_size_);
                });
        if (get_element_level_) (this->*UpdateMaxLevel)();
    }

    template <UpdateMaxLevelMethod UpdateMaxLevel>
    void WalkToTail(GeneralizationHelper& helper, PathToNode::iterator next_lhs_iter) {
        auto& total_checker = helper.GetTotalChecker();
        while (next_lhs_iter != GetEndLhsIter()) {
            auto const& [next_node_offset, next_lhs_rcv_id] = *next_lhs_iter;
            ++next_lhs_iter;
            auto add_normal = [&](MdeNode& node) {
                AddNewMinimal<UpdateMaxLevel>(node, next_lhs_iter);
            };
            if (total_checker.HasGeneralizationInChildren(helper.CurNode(), next_lhs_iter,
                                                          next_node_offset + 1))
                return;
            if (helper.SetAndCheck(TryGetNextNode(helper, next_node_offset, add_normal,
                                                  next_lhs_rcv_id, next_lhs_iter)))
                return;
        }
        helper.SetRhsOnCurrent();
    }

    void AddIfMinimalAppend(GeneralizationHelper& helper) {
        DESBORDANTE_ASSUME(GetSpecLhsIter() == GetEndLhsIter());
        auto const& [end_node_offset, new_node_rcv_id] = GetNewChildNode();

        auto set_and_update_max_level = [this](MdeNode& node) {
            assert(node.IsEmpty());
            node.SetRhs(GetRhs());
            UpdateMaxLevelAdd();
        };
        if (helper.SetAndCheck(TryGetNextNode(helper, end_node_offset, set_and_update_max_level,
                                              new_node_rcv_id, GetEndLhsIter())))
            return;
        helper.SetRhsOnCurrent();
    }

    void AddIfMinimalReplace(GeneralizationHelper& helper) {
        DESBORDANTE_ASSUME(GetSpecLhsIter() != GetEndLhsIter());
        auto const& [new_node_offset, new_node_rcv_id] = GetNewChildNode();
        auto const& [old_node_offset, old_node_rcv_id] = *GetSpecLhsIter();
        DESBORDANTE_ASSUME(new_node_offset == old_node_offset);
        DESBORDANTE_ASSUME(old_node_rcv_id < new_node_rcv_id);

        DESBORDANTE_ASSUME(!helper.Children()[old_node_offset].empty());
        // clang-tidy false positive bypass.
        auto get_higher = [old_node_rcv_id = old_node_rcv_id](MdeRCVIdChildMap& child_map) {
            return child_map.upper_bound(old_node_rcv_id);
        };
        PathToNode::iterator spec_iter = std::next(GetSpecLhsIter());
        auto add_normal = [&](MdeNode& node) {
            AddNewMinimal<&Specializer::UpdateMaxLevelReplace>(node, spec_iter);
        };
        if (helper.SetAndCheck(TryGetNextNodeChildMap(helper.Children()[old_node_offset], helper,
                                                      new_node_offset, add_normal, new_node_rcv_id,
                                                      spec_iter, get_higher)))
            return;
        WalkToTail<&Specializer::UpdateMaxLevelReplace>(helper, spec_iter);
    }

    void AddIfMinimalInsert(GeneralizationHelper& helper) {
        DESBORDANTE_ASSUME(GetSpecLhsIter() != GetEndLhsIter());
        auto const& [new_next_node_offset, new_next_node_rcv_id] = GetNewChildNode();
        auto const& [old_lhs_next_node_offset, old_lhs_next_node_rcv_id] = *GetSpecLhsIter();
        DESBORDANTE_ASSUME(old_lhs_next_node_offset > new_next_node_offset);

        // Number that, when added to the old offset of the next node, gives its new offset.
        std::size_t const old_offset_addend = -(new_next_node_offset + 1);
        std::size_t const old_lhs_next_node_new_offset =
                old_lhs_next_node_offset + old_offset_addend;

        auto add_old_nodes_with_correction = [&](MdeNode& node) {
            AddNewMinimal<&Specializer::UpdateMaxLevelAdd>(
                    *node.AddOneUnchecked(old_lhs_next_node_new_offset, old_lhs_next_node_rcv_id,
                                          record_matches_size_),
                    std::next(GetSpecLhsIter()));
        };

        auto& total_checker = helper.GetTotalChecker();
        if (helper.SetAndCheck(TryGetNextNode(helper, new_next_node_offset,
                                              add_old_nodes_with_correction, new_next_node_rcv_id,
                                              GetSpecLhsIter(), old_offset_addend)))
            return;
        if (total_checker.HasGeneralizationInChildren(helper.CurNode(), GetSpecLhsIter(),
                                                      old_offset_addend))
            return;
        PathToNode::iterator spec_iter = std::next(GetSpecLhsIter());
        auto add_normal = [&](MdeNode& node) {
            AddNewMinimal<&Specializer::UpdateMaxLevelAdd>(node, spec_iter);
        };
        if (helper.SetAndCheck(TryGetNextNode(helper, old_lhs_next_node_new_offset, add_normal,
                                              old_lhs_next_node_rcv_id, spec_iter)))
            return;
        WalkToTail<&Specializer::UpdateMaxLevelAdd>(helper, spec_iter);
    }

    void SetLhsRCVID() {
        GetNewChildNode().rcv_id = get_lhs_rcv_id_(lhs_spec_index_, GetSpecLhsIter()->rcv_id) + 1;
    }

    void SetNonLhsRCVID() {
        GetNewChildNode().rcv_id = get_nonlhs_rcv_id_(lhs_spec_index_) + 1;
    }

    void SpecializeReplaceOnly() {
        for (; GetSpecLhsIter() != GetEndLhsIter(); AdvanceLhsSpecIter(), ++lhs_spec_index_) {
            Index const next_node_offset = GetSpecLhsIter()->offset;
            lhs_spec_index_ += next_node_offset;
            GetNewChildNode().offset = next_node_offset;
            SetLhsRCVID();
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
                SetNonLhsRCVID();
                SpecializeElement<&Specializer::IsUnsupportedNonReplace,
                                  &SpecGenCheckerType::HasGeneralizationInChildrenNonReplace,
                                  &Specializer::AddIfMinimalInsert>();
                ++lhs_spec_index_;
            }
            SetLhsRCVID();
            SpecializeElement<&Specializer::IsUnsupportedReplace,
                              &SpecGenCheckerType::HasGeneralizationInChildrenReplace,
                              &Specializer::AddIfMinimalReplace>();
        }
        for (spec_child_index = 0; lhs_spec_index_ != record_matches_size_; ++spec_child_index) {
            SetNonLhsRCVID();
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

    PathToNode const& GetLhs() noexcept {
        return GetLhsSpecialization().old_lhs;
    }

    PathToNode::iterator GetSpecLhsIter() noexcept {
        return GetLhsSpecialization().specialization_data.spec_before;
    }

    PathToNode::iterator GetEndLhsIter() noexcept {
        return GetLhs().end();
    }

    void AdvanceLhsSpecIter() noexcept {
        ++current_specialization_.GetLhsSpecialization().specialization_data.spec_before;
    }

    std::vector<std::size_t> GetElementLevels() {
        std::vector<std::size_t> element_levels =
                util::GetPreallocatedVector<std::size_t>(GetLhs().PathLength());
        Index record_match_index = 0;
        for (auto const& [offset, rcv_id] : GetLhs()) {
            record_match_index += offset;
            element_levels.push_back(get_element_level_(rcv_id, record_match_index));
            ++record_match_index;
        }
        return element_levels;
    }

    RecordClassifierValueId GetNewRCVId() {
        return GetNewChildNode().rcv_id;
    }

    PathStep& GetNewChildNode() {
        return current_specialization_.GetLhsSpecialization().specialization_data.new_child;
    }

    MdeNode& mde_root_;
    SupportNode& support_root_;
    SingleLevelFunc const& get_element_level_;
    std::size_t& max_level_;
    std::size_t const cardinality_limit_;
    std::size_t const record_matches_size_;
    std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps_;
    FGetLhsRCVId get_lhs_rcv_id_;
    FGetNonLhsRCVId get_nonlhs_rcv_id_;
    bool const prune_nondisjoint_;

    MdeInfoType current_specialization_;

    std::vector<std::size_t> const element_levels_ = GetElementLevels();
    std::size_t const base_level_ =
            std::accumulate(element_levels_.begin(), element_levels_.end(), 0);

    Index lhs_spec_index_ = 0;
    PathToNode::iterator const lhs_end_ = GetLhs().end();

public:
    Specializer(MdeNode& mde_root, SupportNode& support_root,
                SingleLevelFunc const& get_element_level, std::size_t& max_level,
                std::size_t const cardinality_limit, std::size_t const record_matches_size,
                std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps,
                PathToNode const& lhs, FGetLhsRCVId get_lhs_rcv_id,
                FGetNonLhsRCVId get_nonlhs_rcv_id, bool prune_nondisjoint, RhsType rhs)
        : mde_root_(mde_root),
          support_root_(support_root),
          get_element_level_(get_element_level),
          max_level_(max_level),
          cardinality_limit_(cardinality_limit),
          record_matches_size_(record_matches_size),
          rcv_id_lr_maps_(rcv_id_lr_maps),
          get_lhs_rcv_id_(std::move(get_lhs_rcv_id)),
          get_nonlhs_rcv_id_(std::move(get_nonlhs_rcv_id)),
          prune_nondisjoint_(prune_nondisjoint),
          current_specialization_(
                  {LhsSpecialization{lhs, SpecializationData{lhs.begin(), PathStep{}}}, rhs}) {}

    void Specialize() {
        if (GetLhs().PathLength() == cardinality_limit_) {
            SpecializeReplaceOnly();
            return;
        }
        SpecializeFull();
    }
};
}  // namespace

namespace algos::hymde::cover_calculation::lattice {
// TODO: remove recursion
MdeLattice::MdeLattice(SingleLevelFunc single_level_func,
                       std::vector<record_match_indexes::RcvIdLRMap> const& rcv_id_lr_maps,
                       bool prune_nondisjoint, std::size_t max_cardinality, Rhs max_rhs)
    : record_matches_size_(rcv_id_lr_maps.size()),
      mde_root_(record_matches_size_, std::move(max_rhs)),
      support_root_(record_matches_size_),
      get_element_level_(std::move(single_level_func)),
      rcv_id_lr_maps_(&rcv_id_lr_maps),
      prune_nondisjoint_(prune_nondisjoint),
      cardinality_limit_(max_cardinality) {
    enabled_rhs_indices_.resize(record_matches_size_, true);
}

inline void MdeLattice::Specialize(PathToNode const& lhs,
                                   PairComparisonResult const& pair_comparison_result,
                                   Rhss const& rhss) {
    auto get_pair_lhs_rcv_id = [this, pair_comparison_result](Index index, ...) {
        return (*rcv_id_lr_maps_)[index].rhs_to_lhs_map[pair_comparison_result.rhss[index]];
    };
    Specialize(lhs, rhss, get_pair_lhs_rcv_id, get_pair_lhs_rcv_id);
}

inline void MdeLattice::Specialize(PathToNode const& lhs, Rhss const& rhss) {
    auto get_lowest = [](...) { return kLowestRCValueId; };
    auto get_lhs_rcv_id = [](Index, RecordClassifierValueId rcv_id) { return rcv_id; };
    Specialize(lhs, rhss, get_lhs_rcv_id, get_lowest);
}

template <typename MdeInfoType>
auto MdeLattice::CreateSpecializer(PathToNode const& lhs, auto&& rhs, auto get_lhs_rcv_id,
                                   auto get_nonlhs_rcv_id) {
    return Specializer<MdeInfoType, decltype(get_lhs_rcv_id), decltype(get_nonlhs_rcv_id)>(
            mde_root_, support_root_, get_element_level_, max_level_, cardinality_limit_,
            record_matches_size_, *rcv_id_lr_maps_, lhs, get_lhs_rcv_id, get_nonlhs_rcv_id,
            prune_nondisjoint_, rhs);
}

void MdeLattice::Specialize(PathToNode const& lhs, Rhss const& rhss, auto get_lhs_rcv_id,
                            auto get_nonlhs_rcv_id) {
    switch (rhss.size()) {
        case 0:
            break;
        case 1:
            CreateSpecializer<MdeSpecialization>(lhs, rhss.front(), get_lhs_rcv_id,
                                                 get_nonlhs_rcv_id)
                    .Specialize();
            break;
        default: {
            auto enabled_rhss =
                    utility::ExclusionList<MdeElement>::Create(rhss, enabled_rhs_indices_);
            CreateSpecializer<MultiMdeSpecialization>(lhs, enabled_rhss, get_lhs_rcv_id,
                                                      get_nonlhs_rcv_id)
                    .Specialize();
        }
    }
}

std::size_t MdeLattice::PairUpdater::Refine() {
    std::size_t removed = 0;
    Rhs& rhs = node_location_.node->rhs;
    for (auto const& new_rhs : invalidated_.GetUpdateView()) {
        auto const& [rhs_index, new_rcv_id] = new_rhs;
        DESBORDANTE_ASSUME(rhs.begin[rhs_index] != kLowestRCValueId);
        rhs.Set(rhs_index, kLowestRCValueId);
        bool const trivial = new_rcv_id == kLowestRCValueId;
        if (trivial) {
            ++removed;
            continue;
        }
        bool const not_minimal = lattice_->HasGeneralization({GetLhs(), new_rhs});
        if (not_minimal) {
            ++removed;
            continue;
        }
        DESBORDANTE_ASSUME(rhs.begin[rhs_index] == kLowestRCValueId &&
                           new_rcv_id != kLowestRCValueId);
        rhs.Set(rhs_index, new_rcv_id);
    }
    lattice_->Specialize(GetLhs(), *pair_comparison_result_, invalidated_.GetInvalidated());
    if (rhs.IsEmpty() && node_location_.node->IsEmpty()) {
        lattice_->TryDeleteEmptyNode<false>(GetLhs());
    }
    return removed;
}

void MdeLattice::TryAddRefiner(std::vector<PairUpdater>& found, MdeNode& cur_node,
                               PairComparisonResult const& pair_comparison_result,
                               PathToNode const& cur_node_lhs) {
    Rhs& rhs = cur_node.rhs;
    ValidationRhsUpdates invalidated;
    Index rhs_index = 0;
    Index cur_lhs_index = 0;
    auto try_push_no_match_classifier = [&]() {
        RecordClassifierValueId pair_rcv_id = pair_comparison_result.rhss[rhs_index];
        RecordClassifierValueId rhs_rcv_id = rhs[rhs_index];
        if (pair_rcv_id < rhs_rcv_id) {
            invalidated.PushBack({rhs_index, rhs_rcv_id}, pair_rcv_id);
        }
    };
    for (auto const& [child_index, lhs_rcv_id] : cur_node_lhs) {
        cur_lhs_index += child_index;
        for (; rhs_index != cur_lhs_index; ++rhs_index) {
            try_push_no_match_classifier();
        }
        DESBORDANTE_ASSUME(rhs_index < record_matches_size_);
        RecordClassifierValueId const pair_rcv_id = pair_comparison_result.rhss[rhs_index];
        RecordClassifierValueId const rhs_rcv_id = rhs[rhs_index];
        if (pair_rcv_id < rhs_rcv_id) {
            MdeElement invalid{rhs_index, rhs_rcv_id};
            RecordClassifierValueId cur_lhs_triviality_bound =
                    (*rcv_id_lr_maps_)[cur_lhs_index].lhs_to_rhs_map[lhs_rcv_id];
            if (cur_lhs_triviality_bound == pair_rcv_id) {
                invalidated.PushBack(invalid, kLowestRCValueId);
            } else {
                DESBORDANTE_ASSUME(pair_rcv_id > cur_lhs_triviality_bound);
                invalidated.PushBack(invalid, pair_rcv_id);
            }
        }
        ++rhs_index;
        ++cur_lhs_index;
    }
    for (; rhs_index != record_matches_size_; ++rhs_index) {
        try_push_no_match_classifier();
    }
    if (invalidated.IsEmpty()) return;
    found.emplace_back(this, &pair_comparison_result, MdeNodeLocation{cur_node_lhs, &cur_node},
                       std::move(invalidated));
}

void MdeLattice::CollectRefinersForViolated(MdeNode& cur_node, std::vector<PairUpdater>& found,
                                            PathToNode& cur_node_lhs,
                                            PathToNode::iterator cur_lhs_iter,
                                            PairComparisonResult const& pair_comparison_result) {
    if (!cur_node.rhs.IsEmpty()) {
        TryAddRefiner(found, cur_node, pair_comparison_result, cur_node_lhs);
    }

    Index total_offset = 0;
    for (PathToNode::iterator const end = pair_comparison_result.maximal_matching_lhs.end();
         cur_lhs_iter != end; ++total_offset) {
        auto const& [next_node_offset, generalization_rcv_id_limit] = *cur_lhs_iter;
        ++cur_lhs_iter;
        total_offset += next_node_offset;
        RecordClassifierValueId& cur_lhs_rcv_id = cur_node_lhs.NextStep(total_offset);
        for (auto& [generalization_rcv_id, node] : cur_node.children[total_offset]) {
            if (generalization_rcv_id > generalization_rcv_id_limit) break;
            cur_lhs_rcv_id = generalization_rcv_id;
            CollectRefinersForViolated(node, found, cur_node_lhs, cur_lhs_iter,
                                       pair_comparison_result);
        }
        cur_node_lhs.RemoveLastStep();
    }
}

auto MdeLattice::CollectRefinersForViolated(PairComparisonResult const& pair_comparison_result)
        -> std::vector<PairUpdater> {
    std::vector<PairUpdater> found;
    PathToNode current_path(pair_comparison_result.maximal_matching_lhs.PathLength());
    CollectRefinersForViolated(mde_root_, found, current_path,
                               pair_comparison_result.maximal_matching_lhs.begin(),
                               pair_comparison_result);
    // TODO: traverse support trie simultaneously.
    util::EraseIfReplace(found, [this](PairUpdater& refiner) {
        bool const unsupported = IsUnsupported(refiner.GetLhs());
        if (unsupported) TryDeleteEmptyNode<true>(refiner.GetLhs());
        return unsupported;
    });
    return found;
}

template <bool MayNotExist>
void MdeLattice::TryDeleteEmptyNode(PathToNode const& lhs) {
    std::vector<PathElement> path_to_node =
            util::GetPreallocatedVector<PathElement>(lhs.PathLength());
    MdeNode* cur_node_ptr = &mde_root_;
    for (auto const& [node_offset, rcv_id] : lhs) {
        auto& map = cur_node_ptr->children[node_offset];
        auto it = map.find(rcv_id);
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

bool MdeLattice::IsUnsupported(PathToNode const& lhs) const {
    return TotalGeneralizationChecker<SupportNode>{lhs}.HasGeneralization(support_root_);
}

void MdeLattice::ValidationUpdater::MarkUnsupported() {
    // TODO: specializations can be removed from the lattice. If not worth it, removing just this
    // node and its children should be cheap. Though, destructors also take time.

    // This matters. Violation search can find a node with a specialized LHS but higher RHS record
    // classifier value ID, leading to extra work (though no influence on correctness, as MDEs with
    // unsupported LHSs are filtered out).

    lattice_->MarkUnsupported(GetLhs());
    lattice_->TryDeleteEmptyNode<true>(GetLhs());
}

void MdeLattice::ValidationUpdater::LowerAndSpecialize(ValidationRhsUpdates const& invalidated) {
    Rhs& rhs = GetRhs();
    for (auto [rhs_index, new_rcv_id] : invalidated.GetUpdateView()) {
        DESBORDANTE_ASSUME(rhs[rhs_index] != kLowestRCValueId);
        rhs.Set(rhs_index, new_rcv_id);
    }
    lattice_->Specialize(GetLhs(), invalidated.GetInvalidated());
    if (rhs.IsEmpty() && GetNode().IsEmpty()) {
        lattice_->TryDeleteEmptyNode<false>(GetLhs());
    }
}

// The original paper mentions checking for the case where all decision bounds are 1.0, but if such
// a situation occurs for any one RHS, and the generalization with that RHS happens to be valid on
// the data, it would make inference from record pairs give an incorrect result, meaning the
// algorithm is incorrect. However, it is possible to stop traversing when the bound's index in the
// list of natural decision boundaries (that being record classifier value ID) is exactly one less
// than the RHS bound's index, which is done here.
void MdeLattice::RaiseInterestingnessRCVIds(
        MdeNode const& cur_node, PathToNode const& lhs,
        std::vector<RecordClassifierValueId>& cur_interestingness_rcv_ids,
        PathToNode::iterator cur_lhs_iter, std::vector<Index> const& indices,
        std::vector<RecordClassifierValueId> const& rcv_id_bounds, std::size_t& max_count) const {
    std::size_t const indices_size = indices.size();
    if (!cur_node.rhs.IsEmpty()) {
        for (Index i : utility::IndexRange(indices_size)) {
            RecordClassifierValueId const cur_node_rhs_rcv_id = cur_node.rhs[indices[i]];
            RecordClassifierValueId& cur_interestingness_rcv_id = cur_interestingness_rcv_ids[i];
            if (cur_node_rhs_rcv_id > cur_interestingness_rcv_id) {
                cur_interestingness_rcv_id = cur_node_rhs_rcv_id;
                if (cur_interestingness_rcv_id == rcv_id_bounds[i]) {
                    ++max_count;
                    if (max_count == indices_size) return;
                }
            }
        }
    }

    Index total_offset = 0;
    for (PathToNode::iterator const end = lhs.end(); cur_lhs_iter != end; ++total_offset) {
        auto const& [next_node_offset, generalization_rcv_id_limit] = *cur_lhs_iter;
        ++cur_lhs_iter;
        total_offset += next_node_offset;
        // TODO: store iterator in node path, iterate from .begin() to it.
        for (auto const& [generalization_rcv_id, node] : cur_node.children[total_offset]) {
            if (generalization_rcv_id > generalization_rcv_id_limit) break;
            RaiseInterestingnessRCVIds(node, lhs, cur_interestingness_rcv_ids, cur_lhs_iter,
                                       indices, rcv_id_bounds, max_count);
            if (max_count == indices_size) return;
        }
    }
}

std::vector<RecordClassifierValueId> MdeLattice::GetInterestingnessRCVIds(
        PathToNode const& lhs, std::vector<Index> const& indices,
        std::vector<RecordClassifierValueId> const& rcv_id_bounds) const {
    std::vector<RecordClassifierValueId> interestingness_rcv_ids;
    std::size_t const indices_size = indices.size();
    if (prune_nondisjoint_) {
        interestingness_rcv_ids.assign(indices_size, kLowestRCValueId);
    } else {
        interestingness_rcv_ids.reserve(indices_size);
        assert(std::is_sorted(indices.begin(), indices.end()));
        auto fill_interestingness_rcv_ids = [&]() {
            auto index_it = indices.begin(), index_end = indices.end();
            Index cur_index = 0;
            DESBORDANTE_ASSUME(!indices.empty());
            for (auto const& [child_index, lhs_rcv_id] : lhs) {
                cur_index += child_index;
                Index index;
                while ((index = *index_it) < cur_index) {
                    assert((*rcv_id_lr_maps_)[index].lhs_to_rhs_map[kLowestRCValueId] ==
                           kLowestRCValueId);
                    interestingness_rcv_ids.push_back(kLowestRCValueId);
                    if (++index_it == index_end) return;
                }
                if (cur_index == index) {
                    interestingness_rcv_ids.push_back(
                            (*rcv_id_lr_maps_)[index].lhs_to_rhs_map[lhs_rcv_id]);
                    if (++index_it == index_end) return;
                }
                ++cur_index;
            }
            while (index_it != index_end) {
                assert((*rcv_id_lr_maps_)[*index_it].lhs_to_rhs_map[kLowestRCValueId] ==
                       kLowestRCValueId);
                interestingness_rcv_ids.push_back(kLowestRCValueId);
                ++index_it;
            }
        };
        fill_interestingness_rcv_ids();
    }
    std::size_t max_count = 0;
    for (auto [interestingness_rcv_id, rcv_id_bound] :
         utility::Zip(interestingness_rcv_ids, rcv_id_bounds)) {
        if (interestingness_rcv_id == rcv_id_bound) ++max_count;
    }
    if (max_count == indices_size) return interestingness_rcv_ids;

    RaiseInterestingnessRCVIds(mde_root_, lhs, interestingness_rcv_ids, lhs.begin(), indices,
                               rcv_id_bounds, max_count);
    return interestingness_rcv_ids;
}

bool MdeLattice::HasGeneralization(Mde const& mde) const {
    return MdGenChecker<Mde>{mde}.HasGeneralization(mde_root_);
}

void MdeLattice::GetLevel(MdeNode& cur_node, std::vector<ValidationUpdater>& collected,
                          PathToNode& cur_node_lhs, Index const cur_node_record_match_index,
                          std::size_t const level_left) {
    if (level_left == 0) {
        if (!cur_node.rhs.IsEmpty())
            collected.emplace_back(this, MdeNodeLocation{cur_node_lhs, &cur_node});
        return;
    }
    auto collect = [&](MdeRCVIdChildMap& child_map, Index next_node_offset) {
        Index const next_node_record_match_index = cur_node_record_match_index + next_node_offset;
        RecordClassifierValueId& next_lhs_rcv_id = cur_node_lhs.NextStep(next_node_offset);
        for (auto& [rcv_id, node] : child_map) {
            std::size_t const element_level =
                    get_element_level_(next_node_record_match_index, rcv_id);
            if (element_level > level_left) break;
            next_lhs_rcv_id = rcv_id;
            GetLevel(node, collected, cur_node_lhs, next_node_record_match_index + 1,
                     level_left - element_level);
        }
        cur_node_lhs.RemoveLastStep();
    };
    cur_node.ForEachNonEmpty(collect);
}

auto MdeLattice::GetLevel(std::size_t const level) -> std::vector<ValidationUpdater> {
    std::vector<ValidationUpdater> collected;
    PathToNode current_lhs(record_matches_size_);
    if (!get_element_level_) {
        GetAll(mde_root_, current_lhs,
               [this, &collected](PathToNode& cur_node_lhs, MdeNode& cur_node) {
                   collected.emplace_back(this, MdeNodeLocation{cur_node_lhs, &cur_node});
               });
    } else {
        GetLevel(mde_root_, collected, current_lhs, 0, level);
    }
    // TODO: traverse support trie simultaneously.
    util::EraseIfReplace(collected, [this](ValidationUpdater& messenger) {
        bool is_unsupported = IsUnsupported(messenger.GetLhs());
        if (is_unsupported) TryDeleteEmptyNode<true>(messenger.GetLhs());
        return is_unsupported;
    });
    return collected;
}

void MdeLattice::GetAll(MdeNode& cur_node, PathToNode& cur_node_lhs, auto&& add_node) {
    if (!cur_node.rhs.IsEmpty()) add_node(cur_node_lhs, cur_node);

    auto collect = [&](MdeRCVIdChildMap& child_map, Index next_node_offset) {
        RecordClassifierValueId& next_lhs_rcv_id = cur_node_lhs.NextStep(next_node_offset);
        for (auto& [rcv_id, node] : child_map) {
            next_lhs_rcv_id = rcv_id;
            GetAll(node, cur_node_lhs, add_node);
        }
        cur_node_lhs.RemoveLastStep();
    };
    cur_node.ForEachNonEmpty(collect);
}

std::vector<MdeNodeLocation> MdeLattice::GetAll() {
    std::vector<MdeNodeLocation> collected;
    PathToNode current_lhs(record_matches_size_);
    GetAll(mde_root_, current_lhs, [&collected](PathToNode& cur_node_lhs, MdeNode& cur_node) {
        collected.push_back({cur_node_lhs, &cur_node});
    });
    assert(std::ranges::none_of(collected, [this](MdeNodeLocation const& node_info) {
        return IsUnsupported(node_info.lhs);
    }));
    return collected;
}

void MdeLattice::MarkNewLhs(SupportNode& cur_node, PathToNode const& lhs,
                            PathToNode::iterator cur_lhs_iter) {
    AddUnchecked(&cur_node, lhs, cur_lhs_iter, MarkUnsupportedAction());
}

void MdeLattice::MarkUnsupported(PathToNode const& lhs) {
    auto mark_new = [this](auto&&... args) { MarkNewLhs(std::forward<decltype(args)>(args)...); };
    CheckedAdd(&support_root_, lhs, lhs, mark_new, MarkUnsupportedAction());
}
}  // namespace algos::hymde::cover_calculation::lattice
