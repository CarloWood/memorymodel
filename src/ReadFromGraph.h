#pragma once

#include "PathConditionPerLoopEvent.h"
#include "ReadFromLocationSubgraphs.h"
#include "TopologicalOrderedActions.h"
#include "RFLocationOrderedSubgraphs.h"
#include "ast_tag.h"

class ReadFromGraph : public DirectedSubgraph
{
 private:
  using set_type = uint32_t;

  struct NodeData {
    set_type m_set;                                             // The set type (unvisited, followed, processed (dead_end, current_cycle or dead_cycle)) of each node.
    int m_end_point;                                            // The end point of a detected cycle (or -1 if it's a dead_end).
    PathConditionPerLoopEvent m_path_condition_per_loop_event;  // Helper variables to calculate the condition under which a loop exists.
    NodeData() : m_set(0) { }
  };

  int const m_number_of_nodes;                                  // Copy of DirectedSubgraph::m_nodes.size().
  set_type m_generation;                                        // The current generation.
  boolean::Expression m_loop_condition;                         // Collector for the total condition under which there is any loop.
  RFLocationOrderedSubgraphs m_current_subgraphs;               // List of subgraphs that make up the current graph.
  utils::Vector<NodeData, SequenceNumber> m_node_data;          // The node data, using the nodes id as index.
  std::vector<SequenceNumber> m_last_write_per_location;        // Keeps track of the last node that wrote to a given memory location (iend when nothing was written yet).
  TopologicalOrderedActions const& m_topological_ordered_actions;    // Maps node sequence numbers to Action objects.
  std::vector<RFLocation> m_location_tag_to_current_subgraphs_index_map;// Maps location tags to an index into m_current_subgraphs.

 public:
  // Reset all nodes to the state 'unvisited'.
  void reset() { m_generation += 3; }

  // Return true if node n is not yet visited.
  bool is_unvisited(SequenceNumber n) const { return m_node_data[n].m_set <= m_generation; }

  // Mark node n as being followed.
  void set_followed(SequenceNumber n) { m_node_data[n].m_set = m_generation + 1; m_node_data[n].m_path_condition_per_loop_event.reset(); }

  // Return true we are currently in the process of following node n's children.
  bool is_followed(SequenceNumber n) const { return m_node_data[n].m_set == m_generation + 1; }

  // Mark node n as being part of a detected cycle.
  void set_cycle(SequenceNumber n) { m_node_data[n].m_set = m_generation + 2; }

  // Return true if node n is part of a current or dead cycle.
  bool is_cycle(SequenceNumber n) const { return m_node_data[n].m_set == m_generation + 2; }

  // Return true if node n is part of a dead cycle.
  bool is_dead_cycle(SequenceNumber n) const { return is_cycle(n) && !m_node_data[n].m_path_condition_per_loop_event.contains_actual_loop_event(this); }

  // Mark node n as (being part of) a dead end.
  void set_dead_end(SequenceNumber n) { m_node_data[n].m_set = m_generation + 3; }

  // Return true if node n is a dead end.
  bool is_dead_end(SequenceNumber n) const { return m_node_data[n].m_set == m_generation + 3; }

  // Constructor.
  ReadFromGraph(
      Graph const& graph,
      EdgeMaskType type,
      TopologicalOrderedActions const& topological_ordered_actions,
      std::vector<ReadFromLocationSubgraphs> const& read_from_location_subgraphs_vector);

  void push(DirectedSubgraph const& directed_subgraph) { m_current_subgraphs.push_back(&directed_subgraph); }
  void pop() { m_current_subgraphs.pop_back(); }

  // Returns the condition under which a loop exists (m_loop_condition).
  boolean::Expression const& loop_detected();

  // Returns the last value calculated by loop_detected().
  boolean::Expression const& loop_condition() const { return m_loop_condition; }

  // Do a Depth-First-Search starting from node n, returning true if and only if we detected a cycle
  // in which case m_loop_condition is set to the (non-zero) condition under which a cycle was found.
  bool dfs(SequenceNumber n, int current_memory_location = 0);
};
