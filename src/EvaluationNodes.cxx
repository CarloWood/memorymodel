#include "sys.h"
#include "EvaluationNodes.h"
#include "Node.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, EvaluationNodes const& evaluation_nodes)
{
  os << '<';
  bool first = true;
  for (auto&& node_ptr : evaluation_nodes.m_node_ptrs)
  {
    if (!first)
      os << ", ";
    os << *node_ptr;
    first = false;
  }
  return os << '>';
}
