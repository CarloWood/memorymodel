#include "sys.h"
#include "debug.h"
#include "ValueComputation.h"
#include "utils/macros.h"
#include <iostream>

char const* operator_str(Operator op)
{
  switch (op)
  {
    AI_CASE_RETURN(multiplicative_mo_mul);
    AI_CASE_RETURN(multiplicative_mo_div);
    AI_CASE_RETURN(additive_ado_add);
    AI_CASE_RETURN(additive_ado_sub);
    AI_CASE_RETURN(shift_so_shl);
    AI_CASE_RETURN(shift_so_shr);
    AI_CASE_RETURN(relational_ro_lt);
    AI_CASE_RETURN(relational_ro_gt);
    AI_CASE_RETURN(relational_ro_ge);
    AI_CASE_RETURN(relational_ro_le);
    AI_CASE_RETURN(equality_eo_eq);
    AI_CASE_RETURN(equality_eo_ne);
    AI_CASE_RETURN(bitwise_and);
    AI_CASE_RETURN(bitwise_exclusive_or);
    AI_CASE_RETURN(bitwise_inclusive_or);
    AI_CASE_RETURN(logical_and);
    AI_CASE_RETURN(logical_or);
  }
}

std::ostream& operator<<(std::ostream& os, Operator op)
{
  return os << operator_str(op);
}

std::ostream& operator<<(std::ostream& os, ValueComputation const& value_computation)
{
  os << "<ValueComputation>";
  return os;
}

void ValueComputation::OP(Operator op, ValueComputation const& rhs)
{
  DoutEntering(dc::valuecomp, "ValueComputation::OP(" << op << ", " << rhs << ").");
}

void ValueComputation::postfix_operator(ast::postfix_operators op)
{
  DoutEntering(dc::valuecomp, "ValueComputation::postfix_operator(" << op << ").");
}

void ValueComputation::prefix_operator(ast::unary_operators op)
{
  DoutEntering(dc::valuecomp, "ValueComputation::prefix_operator(" << op << ").");
}

void ValueComputation::unary_operator(ast::unary_operators op)
{
  DoutEntering(dc::valuecomp, "ValueComputation::unary_operator(" << op << ").");
}

void ValueComputation::conditional_operator(ValueComputation const& true_value, ValueComputation const& false_value)
{
  DoutEntering(dc::valuecomp, "ValueComputation::conditional_operator(" << true_value << ", " << false_value << ").");
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct valuecomp("VALUECOMP");
NAMESPACE_DEBUG_CHANNELS_END
#endif
