#include "sys.h"
#include "NodePtrConditionPair.h"
#include "Node.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, NodePtrConditionPair const& current_head_of_thread)
{
  bool is_conditional = current_head_of_thread.m_condition.conditional();
  if (is_conditional)
    os << '{';
  os << *current_head_of_thread.m_node_ptr;
  if (is_conditional)
    os << " with condition " << current_head_of_thread.m_condition << '}';
  return os;
}

#if 0
void NodePtrConditionPair::connected()
{
  m_node_ptr->thread()->connected();
}
#endif
