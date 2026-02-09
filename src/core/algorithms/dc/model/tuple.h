#pragma once

namespace algos::dc {

//  @brief
//  kS and kT are used to describe tuple_ used in ColumnOperand or Predicate.
//  kMixed can only be used to describe Predicate tuple_ since ColumnOperand's may have different
//  tuple_'s.
//
//  Example:
//  Given predicate: t.State == s.State.
//  Left operand has t tuple involved thus ColumnOperand.tuple_ is set to the given tuple
//  (t -> Tuple::kT). Right operand has tuple_ set to Tuple::kS (s -> Tuple::kS). Since left and
//  right ColumnOperand have different tuple_`s, thus for the whole Predicate.tuple_ is set to
//  Tuple::kMixed.
//
//  Given predicate: s.Col0 > 1.5.
//  Left operand - Tuple::kS.
//  Right operand is a constant value thus it has no tuple.
//  If the Predicate involves only one variable ColumnOperand thus Predicate.tuple_ is
//  the same as the tuple_ of involved variable ColumnOperand.
//  In this case Predicate.tuple_ is set to Tuple::kS.
//
enum class Tuple { kS, kT, kMixed };

}  // namespace algos::dc
