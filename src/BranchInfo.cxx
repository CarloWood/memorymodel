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
    ", condition_index:" << m_condition_index <<
    ", last_full_expression_of_true_branch_index:" << m_last_full_expression_of_true_branch_index <<
    ", last_full_expression_of_false_branch_index:" << m_last_full_expression_of_false_branch_index << '}';
}

BranchInfo::BranchInfo(
    ConditionalBranch const& conditional_branch,
    full_expression_evaluations_type& full_expression_evaluations,
    std::unique_ptr<Evaluation>&& previous_full_expression) :
      m_condition(conditional_branch),
      m_full_expression_evaluations(full_expression_evaluations),
      m_in_true_branch(true),
      m_edge_to_true_branch_added(false),
      m_edge_to_false_branch_added(false),
      m_condition_index(-1),
      m_last_full_expression_of_true_branch_index(-1),
      m_last_full_expression_of_false_branch_index(-1)
{
  // Keep Evaluations that are conditionals alive.
  Dout(dc::branch|dc::fullexpr, "1.MOVING m_previous_full_expression (" << *previous_full_expression << ") to m_full_expression_evaluations!");
  m_condition_index = full_expression_evaluations.size();
  m_full_expression_evaluations.push_back(std::move(previous_full_expression));
}

void BranchInfo::begin_branch_false(std::unique_ptr<Evaluation>&& previous_full_expression)
{
  m_in_true_branch = false;
  if (previous_full_expression)
  {
    Dout(dc::branch|dc::fullexpr, "2.MOVING m_previous_full_expression (" << *previous_full_expression << ") to m_full_expression_evaluations!");
    m_last_full_expression_of_true_branch_index = m_full_expression_evaluations.size();
    m_full_expression_evaluations.push_back(std::move(previous_full_expression));
  }
}

void BranchInfo::end_branch(std::unique_ptr<Evaluation>&& previous_full_expression)
{
  if (previous_full_expression)
  {
    // When we get here, we have one of the following situations:
    //
    //   if (A)
    //   {
    //     ...
    //     x = 0;       // The last full-expression of (the true-branch).
    //   }
    //
    //   if (A)
    //   {
    //   }
    //   else
    //   {
    //     ...
    //     x = 0;       // The last full-expression (of the false-branch).
    //   }
    Dout(dc::branch|dc::fullexpr, "3.MOVING m_previous_full_expression (" << *previous_full_expression << ") to m_full_expression_evaluations!");
    if (m_last_full_expression_of_true_branch_index == -1)
      m_last_full_expression_of_true_branch_index = m_full_expression_evaluations.size();
    else
      m_last_full_expression_of_false_branch_index = m_full_expression_evaluations.size();
    m_full_expression_evaluations.push_back(std::move(previous_full_expression));
  }
}
