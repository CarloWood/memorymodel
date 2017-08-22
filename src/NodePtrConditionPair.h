#pragma once

#include "NodePtr.h"
#include "Condition.h"
#include <iosfwd>

struct NodePtrConditionPair
{
  NodePtr m_node_ptr;
  Condition m_condition;

  NodePtr const& node() const { return m_node_ptr; }
  Condition const& condition() const { return m_condition; }

  NodePtrConditionPair(NodePtr const& node_ptr) : m_node_ptr(node_ptr) { }
  NodePtrConditionPair(NodePtr const& node_ptr, Condition const& condition) : m_node_ptr(node_ptr), m_condition(condition) { }

  friend std::ostream& operator<<(std::ostream& os, NodePtrConditionPair const& node_ptr_condition_pair);
};
