#include "core/algorithms/fd/fun/fun.h"

#include "core/algorithms/fd/lhs_mask_fd_view.h"
#include "core/config/max_lhs/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/util/logger.h"

namespace algos::fd {

// This looks like excessive abstraction.
class FUN::FunQuadruple {
private:
    boost::dynamic_bitset<> candidate_;
    unsigned long count_;
    boost::dynamic_bitset<> quasiclosure_;
    boost::dynamic_bitset<> closure_;

public:
    explicit FunQuadruple(boost::dynamic_bitset<> const& candidate)
        : candidate_(candidate),
          count_(0),
          quasiclosure_(candidate.size()),
          closure_(candidate.size()) {}

    boost::dynamic_bitset<> const& GetCandidate() const {
        return candidate_;
    }

    unsigned long GetCount() const {
        return count_;
    }

    boost::dynamic_bitset<> const& GetClosure() const {
        return closure_;
    }

    boost::dynamic_bitset<> const& GetQuasiclosure() const {
        return quasiclosure_;
    }

    void SetCount(unsigned long new_count) {
        count_ = new_count;
    }

    void SetClosure(boost::dynamic_bitset<> new_closure) {
        closure_ = std::move(new_closure);
    }

    void SetQuasiclosure(boost::dynamic_bitset<> const& new_quasiclosure) {
        quasiclosure_ = new_quasiclosure;
    }

    bool operator==(FunQuadruple const& that) const {
        return candidate_ == that.candidate_;
    }

    bool operator!=(FunQuadruple const& that) const {
        return candidate_ != that.candidate_;
    }

    bool operator<(FunQuadruple const& that) const {
        return candidate_ < that.candidate_;
    }

    FunQuadruple Union(model::Index const& that) const {
        boost::dynamic_bitset<> c = candidate_;
        c.set(that);
        return FunQuadruple(std::move(c));
    }

    FunQuadruple Union(boost::dynamic_bitset<> const& that) const {
        return FunQuadruple(candidate_ | that);
    }

    bool Contains(FunQuadruple const& that) const {
        return that.candidate_.is_subset_of(candidate_);
    }

    bool Contains(boost::dynamic_bitset<> const& that) const {
        return that.is_subset_of(candidate_);
    }
};

FUN::FUN() {
    RegisterOptions();
}

void FUN::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kMaxLhsOpt.GetName()});
}

void FUN::RegisterOptions() {
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
}

void FUN::ResetState() {
    fd_view_ = nullptr;
    fds_.assign(input_table_column_plis_.size(), {});
}

bool FUN::IsKey(FunQuadruple const& l) const {
    return l.GetCount() == input_table_column_plis_.front().GetCachedProbingTable()->size();
}

void FUN::DisplayFD(Level const& l_k_minus_1) {
    for (FunQuadruple const& l : l_k_minus_1) {
        /*  our other algorithms mine l.candidate.GetArity() == 0,
         *  while Metanome's FUN explicitly ignores
         */
        if (l.GetCandidate().count() > max_lhs_) continue;
        util::ForEachIndex(l.GetClosure() - l.GetQuasiclosure(), [&](model::Index rhs) {
            std::deque<boost::dynamic_bitset<>>& lhss = fds_[rhs];
            for (boost::dynamic_bitset<> const& lhs : lhss) {
                if (l.Contains(lhs)) {
                    return;
                }
            }
            lhss.push_back(l.GetCandidate());
        });
    }
}

void FUN::PurePrune(Level const& l_k_minus_1, Level& l_k) const {
    for (auto l = l_k.begin(); l != l_k.end();) {
        bool erased = false;
        for (FunQuadruple const& s : l_k_minus_1) {
            if (l->GetCount() == s.GetCount() && l->Contains(s)) {
                l = l_k.erase(l);
                erased = true;
                break;
            }
        }
        if (!erased) {
            l++;
        }
    }
}

void FUN::ComputeClosure(Level& l_k_minus_1, Level const& l_k) const {
    for (FunQuadruple& l : l_k_minus_1) {
        if (IsKey(l)) {
            continue;
        }
        l.SetClosure(l.GetQuasiclosure());
        util::ForEachIndex(r_prime_ - l.GetQuasiclosure(), [&](model::Index a) {
            if (FastCount(l_k_minus_1, l_k, l.Union(a)) == l.GetCount()) {
                boost::dynamic_bitset<> bs = l.GetClosure();
                l.SetClosure(std::move(bs.set(a)));
            }
        });
    }
}

void FUN::ComputeQuasiClosure(Level const& l_k_minus_1, Level& l_k) const {
    for (FunQuadruple& l : l_k) {
        if (IsKey(l)) {
            l.SetClosure(r_);
        }
        l.SetQuasiclosure(l.GetCandidate());
        for (FunQuadruple const& s : l_k_minus_1) {
            if (l.Contains(s)) {
                l.SetQuasiclosure(l.GetQuasiclosure() | s.GetClosure());
            }
        }
    }
}

unsigned long FUN::Count(boost::dynamic_bitset<> const& l) const {
    size_t first_column_index = l.find_first();

    model::PositionListIndex const* pli = &input_table_column_plis_[first_column_index];

    if (l.count() == 1) {
        return pli->GetNumCluster();
    }

    //  workaround to avoid auto-destruction of plis
    std::unique_ptr<model::PositionListIndex> holder;

    for (size_t i = l.find_next(first_column_index); i != boost::dynamic_bitset<>::npos;
         i = l.find_next(i)) {
        pli->Intersect(&input_table_column_plis_[i]).swap(holder);
        pli = holder.get();
    }

    return pli->GetNumCluster();
}

unsigned long FUN::FastCount(Level const& l_k_minus_1, Level const& l_k,
                             FunQuadruple const& l) const {
    auto position_at_l_k = std::find(l_k.begin(), l_k.end(), l);
    if (position_at_l_k != l_k.end()) {
        return position_at_l_k->GetCount();
    }
    unsigned long max = 0;
    for (FunQuadruple const& l_prime : l_k_minus_1) {
        if (l.Contains(l_prime)) {
            max = std::max(max, l_prime.GetCount());
        }
    }
    return max;
}

auto FUN::GenerateCandidate(Level const& l_k) const -> Level {
    std::set<FunQuadruple> l_k_plus_1;
    for (FunQuadruple const& l_prime : l_k) {
        if (IsKey(l_prime)) {
            continue;
        }
        util::ForEachIndex(r_prime_ - l_prime.GetCandidate(), [&](model::Index a) {
            FunQuadruple l = l_prime.Union(a);
            if (l_k_plus_1.find(l) == l_k_plus_1.end()) {
                l.SetCount(Count(l.GetCandidate()));
                l_k_plus_1.emplace(l);
            }
        });
    }
    return {l_k_plus_1.begin(), l_k_plus_1.end()};
}

void FUN::ExecuteInternal() {
    boost::dynamic_bitset<> scratch(input_table_column_plis_.size());

    r_ = scratch;
    r_prime_ = scratch;
    Level l_k_minus_1{FunQuadruple(scratch)};
    Level l_k;
    for (model::Index a = 0; a != input_table_column_plis_.size(); ++a) {
        scratch.set(a);
        FunQuadruple attribute(scratch);
        scratch.reset(a);
        attribute.SetCount(Count(attribute.GetCandidate()));
        l_k.push_back(attribute);
        r_.set(a);
        if (!IsKey(attribute)) {
            r_prime_.set(a);
        }
        if (attribute.GetCount() == 1) {
            fds_[a].push_back(scratch);
        }
    }

    while (!l_k.empty()) {
        ComputeClosure(l_k_minus_1, l_k);
        ComputeQuasiClosure(l_k_minus_1, l_k);
        DisplayFD(l_k_minus_1);
        PurePrune(l_k_minus_1, l_k);
        l_k_minus_1 = l_k;
        l_k = GenerateCandidate(l_k);
    }
    DisplayFD(l_k_minus_1);

    fd_view_ = std::make_shared<LhsMaskFdView>(table_header_, std::move(fds_));
}

}  // namespace algos::fd
