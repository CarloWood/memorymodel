#pragma once

#include <vector>
#include <iosfwd>

class CurrentHeadOfThread;
class NodePtr;

using EvaluationCurrentHeadsOfThread = std::vector<CurrentHeadOfThread>;
using EvaluationNodePtrs = std::vector<NodePtr>;
std::ostream& operator<<(std::ostream& os, EvaluationCurrentHeadsOfThread const& evaluation_current_heads_of_thread);
std::ostream& operator<<(std::ostream& os, EvaluationNodePtrs const& evaluation_node_ptrs);
