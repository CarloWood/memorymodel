#pragma once

#include "NodePtrConditionPair.h"
#include <vector>
#include <iosfwd>

using EvaluationNodePtrConditionPairs = std::vector<NodePtrConditionPair>;
using EvaluationNodePtrs = std::vector<NodePtr>;
std::ostream& operator<<(std::ostream& os, EvaluationNodePtrConditionPairs const& evaluation_node_ptr_condition_pairs);
std::ostream& operator<<(std::ostream& os, EvaluationNodePtrs const& evaluation_node_ptrs);

EvaluationNodePtrConditionPairs add_condition(EvaluationNodePtrs const& node_ptrs, Condition const& condition);
