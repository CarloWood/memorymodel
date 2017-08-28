#include "sys.h"
#include "BranchInfo.h"
#include "Evaluation.h"
#include "debug.h"
#include <iostream>

void BranchInfo::print_on(std::ostream& os) const
{
  os << "{condition:" << m_condition <<
    ", in_true_branch:" << m_in_true_branch <<
    ", edge_to_true_branch_added:" << m_edge_to_true_branch_added <<
    ", edge_to_false_branch_added:" << m_edge_to_false_branch_added <<
    '}';
}

BranchInfo::BranchInfo(ConditionalBranch const& conditional_branch) :
      m_condition(conditional_branch),
      m_in_true_branch(true),
      m_edge_to_true_branch_added(false),
      m_edge_to_false_branch_added(false)
{
}

void BranchInfo::begin_branch_false()
{
  m_in_true_branch = false;
}

void BranchInfo::end_branch()
{
}
