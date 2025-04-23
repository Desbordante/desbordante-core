#pragma once

#include <variant>

// The user needs to specify the search space construction mechanism in a convenient way.
// The algorithm needs to store the search space in a workable way
// Idea: user specifies calculators, plus some additional LHS-exclusion (we know immediately that
// support = 0, impossibility generalization) and RHS-exclusion (what is the lowest RHS, given this
// LHS triviality/disjointness generalization) relations between
// record matches
// we are going to need the size of decision bounds to set LHS-excluded MDEs to the max value
// immediately :(
// Let's not overgeneralize and restrict ourselves to the case in HyMD and DD search: triviality
// only for the same record match, impossibility only relative to one other record match (<= and >=)
// Ignore impossibility entirely, just let it play out.
// Generic LHS-exclusion: given this LHS, what is the max LHS value for this rec. match? Empty
// method for now
// Generic RHS-exclusion: given this LHS, what is the max RHS value for this rec. match? Assert that
// RHS being asked about is the same as LhsSpecialization's specialized element.
// Allow some specifications to introduce multiple search space elements?
namespace algos::hymde {
// RCV = record classifier value
// Search space specification element is a record match together with:
// RHS only: (value matrix, decision bounds)
// LHS only: (upper set index, decision bounds)
// LHS+RHS: (value matrix, upper set index, RHS decision bounds, LHS-RHS RCV ID conv, prune n-disj.)
// using SearchSpaceSpecification = std::vector<SearchSpaceSpecificationElement>;
}  // namespace algos::hymde
