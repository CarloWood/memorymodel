#pragma once

#include "Branch.h"

struct ConditionalBranch
{
  conditionals_type::iterator m_conditional;
  ConditionalBranch(conditionals_type::iterator const& conditional) : m_conditional(conditional) { }
  Condition operator()(bool conditional_true) const { return Condition(Branch(m_conditional, conditional_true)); }
  friend std::ostream& operator<<(std::ostream& os, ConditionalBranch const& conditional_branch);
};
