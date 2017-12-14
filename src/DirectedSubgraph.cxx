#include "sys.h"
#include "DirectedSubgraph.h"
#include "Graph.h"
#include "utils/MultiLoop.h"
#include <utility>
#include <iostream>

DirectedSubgraph::DirectedSubgraph(Graph const& graph, EdgeMaskType type, boolean::Expression&& condition) : m_condition(std::move(condition))
{
  ASSERT(m_nodes.empty());
  m_nodes.resize(graph.size());
  size_t count = 0;
  for (auto&& action_ptr : graph)
  {
    ASSERT(0 <= action_ptr->sequence_number().get_value() && action_ptr->sequence_number().get_value() < m_nodes.size());
    m_nodes[action_ptr->sequence_number()] = DirectedEdgeTails(type, action_ptr.get());
    ++count;
  }
  ASSERT(count == m_nodes.size());
}

void DirectedSubgraph::add_to(Graph& graph) const
{
  for (DirectedEdgeTails const& directed_edge_tails : m_nodes)
    directed_edge_tails.add_to(graph);
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
