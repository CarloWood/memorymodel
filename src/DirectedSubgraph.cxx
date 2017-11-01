#include "sys.h"
#include "DirectedSubgraph.h"
#include "Graph.h"
#include <iostream>

DirectedSubgraph::DirectedSubgraph(Graph const& graph, EdgeMaskType type, boolean::Expression&& condition) : m_condition(std::move(condition))
{
#ifdef CWDEBUG
  int index = 0;
#endif
  for (auto&& action_ptr : graph)
  {
    // Graph::m_nodes is ordered by Action::m_id and therefore
    // node->id() should monotonically increase here. See also NodeOrder.
    ASSERT(action_ptr->id() == index++);
    m_nodes.emplace_back(type, *action_ptr);
  }
}

std::ostream& operator<<(std::ostream& os, DirectedSubgraph const& directed_subgraph)
{
  char const* sep = "<subgraph>";

  for (auto&& directed_tails : directed_subgraph.m_nodes)
  {
    os << sep << directed_tails;
    sep = ", ";
  }
  return os << "</subgraph>";
}
