#include "sys.h"
#include "NodePtrConditionPair.h"
#include "Node.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, NodePtrConditionPair const& node_ptr_condition_pair)
{
  bool is_conditional = node_ptr_condition_pair.m_condition.conditional();
  if (is_conditional)
    os << '{';
  os << *node_ptr_condition_pair.m_node_ptr;
  if (is_conditional)
    os << " with condition " << node_ptr_condition_pair.m_condition << '}';
  return os;
}
