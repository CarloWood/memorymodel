#pragma once

#include "DirectedSubgraph.h"
#include "ast_tag.h"
#include <vector>

class ReadFromGraph : public DirectedSubgraph
{
 private:
  using set_type = uint32_t;

  struct NodeData {
    set_type m_set;                                             // The set type (unvisited, followed, processed (dead_end, current_cycle or dead_cycle)) of each node.
    int m_end_point;                                            // The end point of a detected cycle (or -1 if it's a dead_end).
    boolean::Expression m_loop_condition;                       // Helper variable to calculate the condition under which a loop exists.
  };

  int const m_number_of_nodes;                                  // Copy of DirectedSubgraph::m_nodes.size().
  set_type m_generation;                                        // The current generation.
  std::vector<NodeData> m_node_data;                            // The node data, using the nodes id as index.
  std::vector<DirectedSubgraph const*> m_current_subgraphs;     // List of subgraphs that make up the current graph.

 public:
  // Reset all nodes to the state 'unvisited'.
  void reset() { m_generation += 3; }

  // Return true if node n is not yet visited.
  bool is_unvisited(int n) const { return m_node_data[n].m_set <= m_generation; }

  // Mark node n as being followed.
  void set_followed(int n) { m_node_data[n].m_set = m_generation + 1; }

  // Return true we are currently in the process of following node n's children.
  bool is_followed(int n) const { return m_node_data[n].m_set == m_generation + 1; }

  // Mark node n as being part of a detected cycle.
  void set_current_cycle(int n, int end_point, boolean::Expression&& loop_condition)
  {
    ASSERT(end_point >= 0 && end_point < m_number_of_nodes);
    m_node_data[n].m_set = m_generation + 2;
    m_node_data[n].m_end_point = end_point;
    m_node_data[n].m_loop_condition = std::move(loop_condition);
  }

  // Return true if node n is part of a current or dead cycle.
  bool is_cycle(int n) const { return m_node_data[n].m_set == m_generation + 2; }

  // Return true if node n is part of one or more current cycles (if any) that were detected.
  bool is_current_cycle(int n) const { return is_cycle(n) && is_followed(m_node_data[n].m_end_point); }

  // Return true if node n is part of a dead cycle.
  bool is_dead_cycle(int n) const { return is_cycle(n) && is_cycle(m_node_data[n].m_end_point); }

  // Mark node n as (being part of) a dead end.
  void set_dead_end(int n) { m_node_data[n].m_set = m_generation + 3; }

  // Return true if node n is a dead end.
  bool is_dead_end(int n) const { return m_node_data[n].m_set == m_generation + 3; }

  ReadFromGraph(Graph const& graph, EdgeMaskType type, boolean::Expression&& condition) :
      DirectedSubgraph(graph, type, std::move(condition)),
      m_number_of_nodes(m_nodes.size()),
      m_generation(0),
      m_node_data(m_number_of_nodes)
  {
    // The opsem subgraph is the first subgraph, and always present.
    push(*this);
  }

  void push(DirectedSubgraph const& directed_subgraph) { m_current_subgraphs.push_back(&directed_subgraph); }
  void pop() { m_current_subgraphs.pop_back(); }

  // Returns the condition under which a loop exists (m_loop_condition).
  boolean::Expression const& loop_detected();

  // Do a Depth-First-Search starting from node n, returning true if and only if we detected a cycle
  // in which case m_loop_condition is set to the (non-zero) condition under which a cycle was found.
  bool dfs(int n, int current_memory_location = 0);
};
