#pragma once

#include <vector>
#include <iosfwd>

class NodePtrConditionPair;
class NodePtr;

using EvaluationNodePtrs = std::vector<NodePtr>;
struct EvaluationNodePtrConditionPairs : public std::vector<NodePtrConditionPair>
{
  EvaluationNodePtrConditionPairs& operator+=(EvaluationNodePtrConditionPairs const& node_ptr_condition_pairs);
  EvaluationNodePtrConditionPairs& operator+=(EvaluationNodePtrs const& node_ptrs);
  EvaluationNodePtrConditionPairs& operator=(EvaluationNodePtrs const& node_ptrs);
};

std::ostream& operator<<(std::ostream& os, EvaluationNodePtrConditionPairs const& evaluation_current_heads_of_thread);
std::ostream& operator<<(std::ostream& os, EvaluationNodePtrs const& evaluation_node_ptrs);
