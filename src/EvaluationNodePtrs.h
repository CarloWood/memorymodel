#pragma once

#include <vector>
#include <iosfwd>

class NodePtrConditionPair;
class NodePtr;

using EvaluationNodePtrConditionPairs = std::vector<NodePtrConditionPair>;
using EvaluationNodePtrs = std::vector<NodePtr>;
std::ostream& operator<<(std::ostream& os, EvaluationNodePtrConditionPairs const& evaluation_current_heads_of_thread);
std::ostream& operator<<(std::ostream& os, EvaluationNodePtrs const& evaluation_node_ptrs);
