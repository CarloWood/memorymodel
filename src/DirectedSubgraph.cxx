#include "sys.h"
#include "DirectedSubgraph.h"
#include "Graph.h"
#include "utils/MultiLoop.h"
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

void DirectedSubgraph::add_to(Graph& graph) const
{
  for (DirectedEdgeTails const& directed_edge_tails : m_nodes)
    directed_edge_tails.add_to(graph);
}

bool DirectedSubgraph::loop_detected(MultiLoop const& ml, std::vector<ReadFromLocationSubgraphs> const& read_from_location_subgraphs) const
{
  DoutEntering(dc::notice, "DirectedSubgraph::loop_detected(" << *ml << ", ...)");
  // The subgraphs themselves are already without loops.
  if (*ml == 0)         // Is this the first (and only) subgraph?
    return false;

  Action
  for (auto&& directed_edge_tails : m_nodes)
    Dout(dc::notice, directed_edge_tails.action());

  return true;
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
