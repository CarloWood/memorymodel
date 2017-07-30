#include "sys.h"
#include "Branches.h"
#include <ostream>

Branches::Branches(Branch const& branch)
{
  m_boolean_product = branch.boolean_product(); // Really just X or ~X (but the latter is stored in a 'Product').
}

void Branches::operator&=(Branches const& branches)
{
  m_boolean_product *= branches.m_boolean_product;
}

std::ostream& operator<<(std::ostream& os, Branches const& branches)
{
  os << branches.m_boolean_product;
  return os;
}
