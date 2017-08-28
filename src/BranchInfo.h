#pragma once

#include "ConditionalBranch.h"
#include <memory>
#include <vector>

class BranchInfo
{
 public:
  //using full_expression_evaluations_type = std::vector<std::unique_ptr<Evaluation>>;

 private:
  ConditionalBranch m_condition;                                        // The conditional of this selection statement.
  bool m_in_true_branch;                                                // True in True-branch, false in False-branch; true or false after selection statement
                                                                        //  depending whether or not we had a False-branch (false if we did).
  bool m_edge_to_true_branch_added;                                     // Set when the edge from the conditional expression was added that represents 'True'.
  bool m_edge_to_false_branch_added;                                    // Set when the edge from the conditional expression was added that represents 'False'.

 public:
  BranchInfo(ConditionalBranch const& conditional_branch);

  void begin_branch_false();
  void end_branch();
  void added_edge_from_condition() { if (m_in_true_branch) m_edge_to_true_branch_added = true; else m_edge_to_false_branch_added = true; }

  bool in_true_branch() const { return m_in_true_branch; }
  bool conditional_edge_of_current_branch_added() const { return m_in_true_branch ? m_edge_to_true_branch_added : m_edge_to_false_branch_added; }
  Condition get_current_condition() const { return m_condition(m_in_true_branch); }
  Condition get_negated_current_condition() const { return m_condition(!m_in_true_branch); }

  void print_on(std::ostream& os) const;
  friend std::ostream& operator<<(std::ostream& os, BranchInfo const& branch_info) { branch_info.print_on(os); return os; }
};


