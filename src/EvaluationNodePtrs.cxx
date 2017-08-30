#include "sys.h"
#include "EvaluationNodePtrs.h"
#include "NodePtrConditionPair.h"
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

std::ostream& operator<<(std::ostream& os, EvaluationNodePtrConditionPairs const& evaluation_node_ptr_condition_pairs)
{
  os << '<';
  bool first = true;
  for (auto&& evaluation_node_ptr_condition_pair : evaluation_node_ptr_condition_pairs)
  {
    if (!first)
      os << ", ";
    os << evaluation_node_ptr_condition_pair;
    first = false;
  }
  return os << '>';
}

EvaluationNodePtrConditionPairs& EvaluationNodePtrConditionPairs::operator+=(EvaluationNodePtrs const& node_ptrs)
{
  for (auto&& node_ptr : node_ptrs)
    emplace_back(node_ptr);
  return *this;
}

EvaluationNodePtrConditionPairs& EvaluationNodePtrConditionPairs::operator=(EvaluationNodePtrs const& node_ptrs)
{
  clear();
  return operator+=(node_ptrs);
}
