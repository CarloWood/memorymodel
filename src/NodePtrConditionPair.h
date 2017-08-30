#pragma once

#include "NodePtr.h"
#include "Condition.h"
#include "Thread.h"
#include <iosfwd>

class NodePtrConditionPair
{
 private:
  NodePtr m_node_ptr;                   // Used as key in EvaluationNodePtrConditionPairs.
  mutable Condition m_condition;        // Info in EvaluationNodePtrConditionPairs.

 public:
  NodePtr const& node() const { return m_node_ptr; }
  Condition const& condition() const { return m_condition; }

  NodePtrConditionPair(NodePtr const& node_ptr) : m_node_ptr(node_ptr) { }
  NodePtrConditionPair(NodePtr const& node_ptr, Condition const& condition) : m_node_ptr(node_ptr), m_condition(condition) { }

  void reset_condition() const { m_condition.reset(); }
  void operator*=(Condition const& condition) const { m_condition *= condition; }

  friend std::ostream& operator<<(std::ostream& os, NodePtrConditionPair const& current_head_of_thread);
};
