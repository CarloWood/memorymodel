#include "sys.h"
#include "Condition.h"
#include <ostream>

Condition::Condition(Branch const& branch)
{
  m_boolean_product = branch.boolean_product(); // Really just X or ~X (but the latter is stored in a 'Product').
}

std::ostream& operator<<(std::ostream& os, Condition const& condition)
{
  os << condition.m_boolean_product;
  return os;
}
