#include "sys.h"
#include "ConditionalBranch.h"
#include "Evaluation.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, ConditionalBranch const& conditional_branch)
{
  os << '{' << *conditional_branch.m_conditional->first << ", " << conditional_branch.m_conditional->second << '}';
  return os;
}


