#pragma once

#include "ast.h"
#include <iosfwd>

enum binary_operators {
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

std::ostream& operator<<(std::ostream& os, binary_operators op);

class ValueComputation
{
 public:
  enum Unused { not_used };

 private:
  enum { unused, uninitialized, literal, variable, unary, binary, condition } m_state;  // See also is_valid.
  bool m_allocated;
  union Simple
  {
    int m_literal;                              // Only valid when m_state == literal.
    ast::tag m_variable;                        // Only valid when m_state == variable.
    Simple() { }
    explicit Simple(int literal) : m_literal(literal) { }
  } m_simple;
  union
  {
    ast::unary_operators unary;                 // Only valid when m_state == unary.
    binary_operators binary;                    // Only valid when m_state == binary.
  } m_operator;
  std::unique_ptr<ValueComputation> m_lhs;      // Only valid when m_state == unary, binary or condition.
  std::unique_ptr<ValueComputation> m_rhs;      // Only valid when m_state == binary or condition.
  std::unique_ptr<ValueComputation> m_condition;// Only valid when m_state == condition  (m_condition ? m_lhs : m_rhs).

 public:
  ValueComputation() : m_state(uninitialized), m_allocated(false) { }
  ValueComputation(ValueComputation&& value_computation);
  explicit ValueComputation(Unused) : m_state(unused), m_allocated(false) { }
  ValueComputation(int value) : m_state(literal), m_allocated(false), m_simple(value) { }
  ValueComputation& operator=(int value) { m_state = literal; m_simple.m_literal = value; return *this; }
  ValueComputation& operator=(ast::tag tag) { m_state = variable; m_simple.m_variable = tag; return *this; }
  void operator=(ValueComputation&& value_computation);

  // Shallow-copy value_computation and turn it into an allocation if it wasn't already.
  static std::unique_ptr<ValueComputation> make_unique(ValueComputation&& value_computation);
  // Apply negation unary operator - to the ValueComputation object pointed to by ptr.
  static void negate(std::unique_ptr<ValueComputation>& ptr);

  bool is_valid() const { return m_state > uninitialized; }
  bool is_sum() const { return m_state == binary && (m_operator.binary == additive_ado_add || m_operator.binary == additive_ado_sub); }
  bool is_negated() const { return m_state == unary && m_operator.unary == ast::uo_minus; }


  void OP(binary_operators op, ValueComputation&& rhs);         // *this OP= rhs.
  void postfix_operator(ast::postfix_operators op);             // (*this)++ or (*this)--
  void prefix_operator(ast::unary_operators op);                // ++*this or --*this
  void unary_operator(ast::unary_operators op);                 // *this = OP *this
  void conditional_operator(ValueComputation&& true_value,      // *this = *this ? true_value : false_value
                            ValueComputation&& false_value);
  void swap_sum();
  void strip_rhs();

  friend std::ostream& operator<<(std::ostream& os, ValueComputation const& value_computation);
};

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct valuecomp;
extern channel_ct simplify;
NAMESPACE_DEBUG_CHANNELS_END
#endif
