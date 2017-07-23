#include "sys.h"
#include "Branch.h"
#include "Evaluation.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, Branch const& branch)
{
  os << "iff " << *branch.m_condition << " == " << (branch.m_condition_true ? "true" : "false") << '}';
  return os;
}
