#include "sys.h"
#include "Condition.h"
#include "Evaluation.h"
#include "Context.h"
#include <ostream>

std::string Condition::id_name() const
{
  return std::string(1, 'A' + m_id);
}

void Condition::create_boolexpr_variable() const
{
  m_boolexpr_variable = boolean_expression::Context::instance().make_variable(0, id_name());
}

std::ostream& operator<<(std::ostream& os, Condition const& condition)
{
  os << '[' << condition.m_id << "] " << *condition.m_condition;
  return os;
}
