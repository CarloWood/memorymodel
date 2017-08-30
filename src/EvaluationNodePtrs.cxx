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

bool NodePtrConditionPairCompare::operator()(NodePtrConditionPair const& pair1, NodePtrConditionPair const& pair2)
{
  return *pair1.node() < *pair2.node();
}

// Combine two sets of node/condition pairs.
// It is possible that two elements have the same key (node), in which case the conditions need to be combined!
EvaluationNodePtrConditionPairs& EvaluationNodePtrConditionPairs::operator+=(EvaluationNodePtrConditionPairs const& node_ptr_condition_pairs)
{
  key_compare comp;
  auto iter1 = begin();
  auto iter2 = node_ptr_condition_pairs.begin();
  while (iter2 != node_ptr_condition_pairs.end())
  {
    if (iter1 == end() || comp(*iter2, *iter1))
    {
      insert(iter1, *iter2++);
      continue;
    }
    if (comp(*iter1, *iter2))
    {
      ++iter1;
      continue;
    }
    // The same node.
    if (iter1->condition() != iter2->condition())
    {
      // The need to combine two unconnected heads of the same node but with different condition
      // should only arise in the case where both branches of a selection statement are empty:
      // if (condition)
      // {
      //   ; // Or just child threads of which at least one is empty.
      // }
      // and thus the conditions should be of a single variable, one negated.
      // Really here we should OR the two conditions (for parallel edges),
      // but the Condition is just a single variable (negated or not);
      // so we can't do that; but we know that the result should be 1.
      // Make sure that above is true.
      ASSERT((boolean::Expression(iter1->condition().boolean_product()) + boolean::Expression(iter2->condition().boolean_product())).is_one());
      iter1->reset_condition();
    }
    ++iter2;
  }
  return *this;
}

EvaluationNodePtrConditionPairs& EvaluationNodePtrConditionPairs::operator+=(EvaluationNodePtrs const& node_ptrs)
{
  for (auto&& node_ptr : node_ptrs)
    emplace(node_ptr);
  return *this;
}

EvaluationNodePtrConditionPairs& EvaluationNodePtrConditionPairs::operator=(EvaluationNodePtrs const& node_ptrs)
{
  clear();
  return operator+=(node_ptrs);
}

void EvaluationNodePtrConditionPairs::operator*=(Condition const& condition)
{
  for (auto&& node_ptr_condition_pair : *this)
    node_ptr_condition_pair *= condition;
}
