#pragma once

#include <vector>
#include <set>
#include <iosfwd>

class NodePtrConditionPair;
class NodePtr;
class Condition;

struct NodePtrConditionPairCompare
{
  bool operator()(NodePtrConditionPair const& pair1, NodePtrConditionPair const& pair2);
};

using EvaluationNodePtrs = std::vector<NodePtr>;
struct EvaluationNodePtrConditionPairs : public std::set<NodePtrConditionPair, NodePtrConditionPairCompare>
{
  EvaluationNodePtrConditionPairs& operator+=(EvaluationNodePtrConditionPairs const& node_ptr_condition_pairs);
  EvaluationNodePtrConditionPairs& operator+=(EvaluationNodePtrs const& node_ptrs);
  EvaluationNodePtrConditionPairs& operator=(EvaluationNodePtrs const& node_ptrs);
  void operator*=(Condition const& condition);
};

std::ostream& operator<<(std::ostream& os, EvaluationNodePtrConditionPairs const& evaluation_current_heads_of_thread);
std::ostream& operator<<(std::ostream& os, EvaluationNodePtrs const& evaluation_node_ptrs);
