#pragma once

#include "ast.h"
#include <iosfwd>

enum Operator {
  multiplicative_mo_mul,
  multiplicative_mo_div,
  additive_ado_add,
  additive_ado_sub,
  shift_so_shl,
  shift_so_shr,
  relational_ro_lt,
  relational_ro_gt,
  relational_ro_ge,
  relational_ro_le,
  equality_eo_eq,
  equality_eo_ne,
  bitwise_and,
  bitwise_exclusive_or,
  bitwise_inclusive_or,
  logical_and,
  logical_or
};

std::ostream& operator<<(std::ostream& os, Operator op);

class ValueComputation
{
 public:
  enum Unused { not_used };

 private:
  enum { unused, uninitialized, valid } m_state;

 public:
  ValueComputation() : m_state(uninitialized) { }
  ValueComputation(Unused) : m_state(unused) { }
  ValueComputation& operator=(int literal) { m_state = valid; return *this; }
  ValueComputation& operator=(bool literal) { m_state = valid; return *this; }
  ValueComputation& operator=(ast::tag variable) { m_state = valid; return *this; }

  void OP(Operator op, ValueComputation const& rhs);    // *this OP= rhs.
  void postfix_operator(ast::postfix_operators op);     // (*this)++ or (*this)--
  void prefix_operator(ast::unary_operators op);        // ++*this or --*this
  void unary_operator(ast::unary_operators op);         // *this = OP *this
  void conditional_operator(ValueComputation const& true_value, ValueComputation const& false_value);    // *this = *this ? true_value : false_value

  friend std::ostream& operator<<(std::ostream& os, ValueComputation const& value_computation);
};

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct valuecomp;
NAMESPACE_DEBUG_CHANNELS_END
#endif
