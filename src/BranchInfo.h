#pragma once

#include "ConditionalBranch.h"
#include <memory>
#include <vector>

class BranchInfo
{
 public:
  using full_expression_evaluations_type = std::vector<std::unique_ptr<Evaluation>>;

 //FIXME private:
  ConditionalBranch m_condition;                                        // The conditional of this selection statement.
  full_expression_evaluations_type& m_full_expression_evaluations;      // Convenience reference to Contect::m_full_expression_evaluations.
  bool m_in_true_branch;                                                // True in True-branch, false in False-branch; true or false after selection statement
                                                                        //  depending whether or not we had a False-branch (false if we did).
  bool m_edge_to_true_branch_added;                                     // Set when the edge from the conditional expression was added that represents 'True'.
  bool m_edge_to_false_branch_added;                                    // Set when the edge from the conditional expression was added that represents 'False'.
  int m_condition_index;                                                // The index into m_full_expression_evaluations of the conditional expression.
  int m_last_full_expression_of_true_branch_index;                      // The index into m_full_expression_evaluations of the last full expression
                                                                        //  of the True-branch, or -1 when it doesn't exist.
  int m_last_full_expression_of_false_branch_index;                     // The index into m_full_expression_evaluations of the last full expression
                                                                        //  of the False-branch, or -1 when it doesn't exist.
 public:
  BranchInfo(
      ConditionalBranch const& conditional_branch,
      full_expression_evaluations_type& full_expression_evaluations,
      std::unique_ptr<Evaluation>&& previous_full_expression);

  void begin_branch_false(std::unique_ptr<Evaluation>&& previous_full_expression);
  void end_branch(std::unique_ptr<Evaluation>&& previous_full_expression);
  void added_edge_from_condition() { if (m_in_true_branch) m_edge_to_true_branch_added = true; else m_edge_to_false_branch_added = true; }

  bool conditional_edge_of_current_branch_added() const { return m_in_true_branch ? m_edge_to_true_branch_added : m_edge_to_false_branch_added; }
  Condition get_current_condition() const { return m_condition(m_in_true_branch); }
  Condition get_negated_current_condition() const { return m_condition(!m_in_true_branch); }

  void print_on(std::ostream& os) const;
  friend std::ostream& operator<<(std::ostream& os, BranchInfo const& branch_info) { branch_info.print_on(os); return os; }
};


