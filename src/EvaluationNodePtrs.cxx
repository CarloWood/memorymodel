#include "sys.h"
#include "EvaluationNodePtrs.h"
#include "CurrentHeadOfThread.h"
#include "Node.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, EvaluationNodePtrs const& evaluation_node_ptrs)
{
  os << '<';
  bool first = true;
  for (auto&& node_ptr : evaluation_node_ptrs)
  {
    if (!first)
      os << ", ";
    os << *node_ptr;
    first = false;
  }
  return os << '>';
}

std::ostream& operator<<(std::ostream& os, EvaluationCurrentHeadsOfThread const& evaluation_current_heads_of_thread)
{
  os << '<';
  bool first = true;
  for (auto&& current_head_of_thread : evaluation_current_heads_of_thread)
  {
    if (!first)
      os << ", ";
    os << current_head_of_thread;
    first = false;
  }
  return os << '>';
}
