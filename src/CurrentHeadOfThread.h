#pragma once

#include "NodePtr.h"
#include "Condition.h"
#include "Thread.h"
#include <iosfwd>

class CurrentHeadOfThread
{
 private:
  NodePtr m_node_ptr;
  Condition m_condition;

 public:
  NodePtr const& node() const { return m_node_ptr; }
  Condition const& condition() const { return m_condition; }
  void connected();

  CurrentHeadOfThread(NodePtr const& node_ptr) : m_node_ptr(node_ptr) { }
  CurrentHeadOfThread(NodePtr const& node_ptr, Condition const& condition) : m_node_ptr(node_ptr), m_condition(condition) { }

  friend std::ostream& operator<<(std::ostream& os, CurrentHeadOfThread const& current_head_of_thread);
};
