#include "sys.h"
#include "debug.h"
#include "ReadFromGraph.h"
#include "Action.h"
#include "Context.h"
#include "utils/MultiLoop.h"

ReadFromGraph::ReadFromGraph(
    Graph const& graph,
    EdgeMaskType outgoing_type,
    EdgeMaskType incoming_type,
    TopologicalOrderedActions const& topological_ordered_actions,
    std::vector<ReadFromLocationSubgraphs> const& read_from_location_subgraphs_vector) :
      DirectedSubgraph(graph, outgoing_type, incoming_type, boolean::Expression{true}),
      m_number_of_nodes(m_nodes.size()),
      m_generation(0),
      m_node_data(m_number_of_nodes),
      m_topological_ordered_actions(topological_ordered_actions),
      m_location_id_to_rf_location(Context::instance().get_position_handler().tag_end())
{
  // The opsem subgraph is the first subgraph, and always present.
  RFLocation index{m_current_subgraphs.ibegin()};
  push(*this);
  // Initialize m_location_id_to_rf_location.
  for (auto&& location_subgraphs : read_from_location_subgraphs_vector)
  {
    Dout(dc::readfrom, location_subgraphs.location() << ':');
    // Subgraphs from read_from_location_subgraphs_vector are pushed in order to m_current_subgraphs
    // by calling push/pop. Therefore the n+1's subgraph in m_current_subgraphs always corresponds
    // to the same memory location, namely the location of the n-th ReadFromLocationSubgraphs
    // of read_from_location_subgraphs_vector. The +1 is because of the push(*this) above.
    m_location_id_to_rf_location[location_subgraphs.location().tag().id] = ++index; // Preincement: we start at index 1.
#ifdef CWDEBUG
    NAMESPACE_DEBUG::Indent indent(4);
#endif
    for (auto&& subgraph : location_subgraphs)
    {
      Dout(dc::readfrom, subgraph);
    }
  }
}

boolean::Expression const& ReadFromGraph::loop_detected()
{
  static int count = 0;
  DoutEntering(dc::notice, "ReadFromGraph::loop_detected() " << ++count);

  // Node 0 is the starting node of the program.
  m_current_node.clear();

  // If no loops are detected when starting from that node then the program
  // has no loops, because every node should be reachable from begin_node.
  // Therefore it is enough to only test this first node.

  // Initialize the condition under which a loop is found to zero.
  m_loop_condition = false;

  if (dfs() && !m_loop_condition.is_zero())
    Dout(dc::notice, "Found inconsistency under condition " << m_loop_condition);

  // Prepare for next call to loop_detected().
  reset();

  return m_loop_condition;
}

// Returns false when no loop was detected starting from node n (and m_node_data[n].m_properties is empty).
// Otherwise returns true and m_node_data[n].m_properties
// contains the non-zero condition(s) under which one or more loop quirks exist.
// m_loop_condition is updated (logical OR-ed) which any full loops that might have been detected.
//
// For example,
//
//         (0)
//          |A
//          v
//      .->(1)
//      |   |B
//      |   v
//      |  (2)
//     E|   |C
//      |   v  F
//      |  (3)--->(5)
//      |   |D     |G
//      |   v   H  v
//      `--(4)<---(6)
//
// Let A, B, ..., H be the boolean expressions under which each edge exists.
// Then dfs(4) is only called after calling dfs(0), dfs(1), dfs(2), dfs(3)
// and then either dfs(4) directly or first dfs(5) and dfs(6), which means
// that node 1 will always be in the 'followed' state; hence that dfs(4) will
// return true and m_node_data[4].m_properties will contain
// the pair {Quirk(causal_loop, 1), E} (lets abbreviate this to {1,E}).
// Therefore, the call to dfs(6) will return true and cause
// m_node_data[6].m_properties to contain {1,HE},
// the call to dfs(5) will return true and cause
// m_node_data[5].m_properties to contain {1,GHE},
// and the call to dfs(3) will return true and cause
// m_node_data[3].m_properties to contain {1,DE + FGHE}.
// The call to dfs(1) will return true and cause
// m_node_data[1].m_properties to contain {1, BCDE + BCFGHE}
// and m_loop_condition to be updated to BCDE + BCFGHE.
// Finally, the call to dfs(0) will return true and but cause
// m_node_data[0].m_properties to remain empty because
// it doesn't contain any still actual loop quirks. The final result (m_loop_condition)
// is therefore BCDE + BCFGHE.
//
// Note that if dfs(4) is called before dfs(5), so that
// m_node_data[4].m_properties is already contains {1,E}; then
// the call to dfs(6) will short circuit the calculation done by dfs(4) and not
// call dfs(4) again because node 1 is still be in the 'followed' state: the
// {1,E} can just be used directly to produce HE. Likewise if the children of
// node 3 are called in the opposite order. But if there would be an edge from
// node 0 to say node 5, then that is considered to be a dead cycle because
// the detected cycle that node 5 is part of "ends" at node 1 which is no longer
// marked as being 'followed' (or rather, the Quirk(causal_loop, 1) will
// no longer return that it is "actual").
//
// The algorithm used here, that detects under which condition a cycle will
// exist when each edge is "weighted" (with a boolean expression in this case),
// was designed by myself (Carlo Wood) in November/December 2017.
//
bool ReadFromGraph::dfs(int current_memory_location)
{
  DoutEntering(dc::readfrom, "ReadFromGraph::dfs(" << current_memory_location << ") for node " << m_current_node << '.');

  // Depth-First search only visits each node once.
  ASSERT(is_unvisited(m_current_node));

  // Current node data.  Facet, Property, Quirk, Trait
  Properties& current_node_data{m_node_data[m_current_node].m_properties};

  // Reset any old values in m_properties (from a previous generation).
  current_node_data.reset();

  // m_current_node equals Action::m_sequence_number, the index for m_topological_ordered_actions.
  ast::tag location = m_topological_ordered_actions[m_current_node]->tag();
  RFLocation rf_location{m_location_id_to_rf_location[location.id]};
  bool const have_location_subgraph = !rf_location.undefined() && rf_location < m_current_subgraphs.iend();
  bool const is_write = have_location_subgraph && m_topological_ordered_actions[m_current_node]->is_write();
  bool const is_read = have_location_subgraph && m_topological_ordered_actions[m_current_node]->is_read();
  SequenceNumber previous_write;

  if (is_read)
  {
    DirectedSubgraph const* subgraph = m_current_subgraphs[rf_location];
    for (auto incoming_read_from_edge = subgraph->edges(m_current_node).begin_incoming();
         incoming_read_from_edge != subgraph->edges(m_current_node).end_incoming();
         ++ incoming_read_from_edge)
    {
      SequenceNumber read_from_node{incoming_read_from_edge->tail_sequence_number()};
      Dout(dc::readfrom, "Node " << m_current_node << " reads from node " << read_from_node);
      current_node_data.add_new(Quirk(reads_from, read_from_node), incoming_read_from_edge->condition().copy());
      Dout(dc::readfrom, "  " << m_current_node << ".properties is now " << current_node_data);
    }
  }

  set_followed(m_current_node);
  int memory_location = 0;
  for (DirectedSubgraph const* subgraph : m_current_subgraphs)
  {
    for (auto directed_edge = subgraph->edges(m_current_node).begin_outgoing(); directed_edge != subgraph->edges(m_current_node).end_outgoing(); ++directed_edge)
    {
      SequenceNumber const child = directed_edge->head_sequence_number();
      Dout(dc::readfrom, "Following edge to child " << child);
      if (directed_edge->is_rf_not_release_acquire())
      {
        // When memory_location == 0 this is a sb or asw edge, not an rf edge.
        ASSERT(memory_location > 0);
        Dout(dc::readfrom, "  (which is a rf edge of memory location '" << memory_location << "' that is not a release-acquire!)");
        if (current_memory_location > 0 && current_memory_location != memory_location)
        {
          Dout(dc::readfrom, "  Continuing because we already encountered a non-rel-acq Read-From edge for memory location " << current_memory_location << "!");
          continue;
        }
        current_memory_location = memory_location;
      }
      if (is_dead_end(child) || is_dead_cycle(child))
      {
        Dout(dc::readfrom, "  continuing because that node is dead.");
        continue;
      }
      if (is_followed(child))
      {
        Dout(dc::readfrom, "  causal loop detected! Marking node " << child << " as end_point.");
        current_node_data.add_new(Quirk(causal_loop, child), directed_edge->condition().copy());
        Dout(dc::readfrom, "  " << m_current_node << ".properties is now " << current_node_data);
      }
      else
      {
        // If is_cycle returns true then we have a current cycle because the cycle isn't dead when we get here.
        bool have_quirks = is_cycle(child);
        if (!have_quirks)
        {
          SequenceNumber current_node{m_current_node};
          m_current_node = child;
          have_quirks = dfs(current_memory_location);
          m_current_node = current_node;
        }
        if (have_quirks)
        {
          Dout(dc::readfrom, "  quirk(s) detected behind child " << child << " of node " << m_current_node <<
              " with properties: " << m_node_data[child].m_properties);
          current_node_data.add_new(m_node_data[child].m_properties, directed_edge->condition(), this);
          if (is_write)
            current_node_data.set_hidden(location, m_topological_ordered_actions);
          Dout(dc::readfrom, "  " << m_current_node << ".properties is now " << current_node_data);
        }
      }
    }
    // When memory_location is larger than 0 the edges being followed
    // are Read-From edges of a certain memory location; different values
    // of memory_location mean different memory locations.
    ++memory_location;
  }
  Dout(dc::readfrom, "Done following children of node " << m_current_node);
  bool have_quirks = !current_node_data.empty();
  if (have_quirks)
  {
    set_cycle(m_current_node);
    boolean::Expression loop_condition = current_node_data.current_loop_condition(m_current_node);
    if (!loop_condition.is_zero())
    {
      Dout(dc::notice, "Violation(s) detected involving end point " << m_current_node << ", under condition " << loop_condition << '.');
      m_loop_condition += loop_condition;
      Dout(dc::readfrom, "m_loop_condition is now: " << m_loop_condition << '.');
    }
  }
  else
    set_dead_end(m_current_node);

  return have_quirks;
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct readfrom("READFROM");
NAMESPACE_DEBUG_CHANNELS_END
#endif
