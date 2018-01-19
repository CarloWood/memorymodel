#include "sys.h"
#include "Conditional.h"
#include <ostream>

std::string Conditional::id_name() const
{
  return std::string(1, 'A' + m_id);
}

std::ostream& operator<<(std::ostream& os, Conditional const& conditional)
{
  os << '[' << conditional.m_id << "] " << conditional.m_boolexpr_variable;
  return os;
}

//static
Conditional::id_type Conditional::s_next_conditional_id;
