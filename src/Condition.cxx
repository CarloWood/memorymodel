#include "sys.h"
#include "Condition.h"
#include "Evaluation.h"
#include "Context.h"
#include <ostream>

std::string Condition::id_name() const
{
  return std::string(1, 'A' + m_id);
}

std::ostream& operator<<(std::ostream& os, Condition const& condition)
{
  os << '[' << condition.m_id << "] " << condition.m_boolexpr_variable;
  return os;
}

//static
Condition::id_type Condition::s_next_condition_id;
