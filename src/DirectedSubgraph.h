#pragma once

#include "DirectedEdges.h"
#include <vector>

class Graph;
class DirectedEdges;
class MultiLoop;
class ReadFromLocationSubgraphs;

class DirectedSubgraph
{
 public:
  using nodes_type = utils::Vector<DirectedEdges, SequenceNumber>;

 protected:
  nodes_type m_nodes;
  boolean::Expression m_condition;              // The condition under which this subgraph is valid.

 public:
  DirectedSubgraph(Graph const& graph, EdgeMaskType outgoing_type, EdgeMaskType incoming_type, boolean::Expression&& condition);

  // Add this subgraph to graph.
  void add_to(Graph& graph) const;
  // Return condition under which this subgraph is valid.
  boolean::Expression const& valid() const { return m_condition; }
  // Return the stored edges (as filtered by incoming_type/outgoing_type) of node n for this subgraph.
  DirectedEdges const& edges(SequenceNumber n) const { return m_nodes[n]; }

  friend std::ostream& operator<<(std::ostream& os, DirectedSubgraph const& directed_subgraph);
};
