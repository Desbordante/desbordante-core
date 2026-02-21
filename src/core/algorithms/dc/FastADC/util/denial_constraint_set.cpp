#include "core/algorithms/dc/FastADC/util/denial_constraint_set.h"

#include "core/algorithms/dc/FastADC/providers/predicate_provider.h"

namespace algos::fastadc {

namespace {

bool ContainedIn(PredicateSet const& ps, PredicateSet const& other_ps,
                 PredicateProvider* provider) {
    for (PredicatePtr p : ps) {
        if (other_ps.Contains(p)) {
            continue;
        }
        if (provider && other_ps.Contains(p->GetSymmetric(provider))) {
            continue;
        }
        return false;
    }
    return true;
}

bool IsEmpty(PredicateSet const& ps) {
    return ps.GetBitset().find_first() == boost::dynamic_bitset<>::npos;
}

std::size_t MaxPredicateHash(PredicatePtr p, PredicateProvider* provider,
                            std::hash<Predicate> const& hasher) {
    auto const p_hash = hasher(*p);
    if (!provider) {
        return p_hash;
    }

    auto const* sym = p->GetSymmetric(provider);
    return std::max(p_hash, hasher(*sym));
}

std::size_t SumMaxPredicateHash(PredicateSet const& ps, PredicateProvider* provider) {
    auto const hasher = std::hash<Predicate>{};

    std::size_t result = 0;
    for (PredicatePtr p : ps) {
        result += MaxPredicateHash(p, provider, hasher);
    }
    return result;
}

}  // namespace

std::size_t DenialConstraintSet::DCHash::operator()(DenialConstraint const& dc) const {
    auto const result1 = SumMaxPredicateHash(dc.GetPredicateSet(), provider);

    if (!provider) {
        return result1;
    }

    auto const inv_dc = dc.GetInvT1T2DC(provider);
    auto const& inv_predicates = inv_dc.GetPredicateSet();
    if (IsEmpty(inv_predicates)) {
        return result1;
    }

    auto const result2 = SumMaxPredicateHash(inv_predicates, provider);
    return std::max(result1, result2);
}

bool DenialConstraintSet::DCEqual::operator()(DenialConstraint const& lhs,
                                             DenialConstraint const& rhs) const {
    if (&lhs == &rhs) {
        return true;
    }

    auto const& lhs_ps = lhs.GetPredicateSet();
    auto const& rhs_ps = rhs.GetPredicateSet();

    if (lhs_ps.Size() != rhs_ps.Size()) {
        return false;
    }

    if (ContainedIn(lhs_ps, rhs_ps, provider)) {
        return true;
    }

    if (!provider) {
        return false;
    }

    auto const lhs_inv_dc = lhs.GetInvT1T2DC(provider);
    auto const& lhs_inv_ps = lhs_inv_dc.GetPredicateSet();
    if (!IsEmpty(lhs_inv_ps) && ContainedIn(lhs_inv_ps, rhs_ps, provider)) {
        return true;
    }

    auto const rhs_inv_dc = rhs.GetInvT1T2DC(provider);
    auto const& rhs_inv_ps = rhs_inv_dc.GetPredicateSet();
    if (!IsEmpty(rhs_inv_ps) && ContainedIn(lhs_ps, rhs_inv_ps, provider)) {
        return true;
    }

    return false;
}

}  // namespace algos::fastadc
