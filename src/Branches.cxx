#include "sys.h"
#include "Branches.h"
#include <ostream>

Branches::Branches(Branch const& branch)
{
  m_boolean_expression = branch.boolean_expression();
}

void Branches::operator&=(Branches const& branches)
{
  m_boolean_expression = boolexpr::and_s({m_boolean_expression, branches.m_boolean_expression});
}

std::ostream& operator<<(std::ostream& os, Branches const& branches)
{
  os << branches.m_boolean_expression->to_string();
  return os;
}

//static
boolexpr::one_t Branches::s_one{boolexpr::one()};
