#include "sys.h"
#include "debug.h"
#include "ReadFromGraph.h"
#include "Action.h"
#include "Context.h"
#include "utils/MultiLoop.h"

ReadFromGraph::ReadFromGraph(
    Graph const& graph,
    EdgeMaskType type,
    TopologicalOrderedActions const& topological_ordered_actions,
    std::vector<ReadFromLocationSubgraphs> const& read_from_location_subgraphs_vector) :
      DirectedSubgraph(graph, type, boolean::Expression{true}),
      m_number_of_nodes(m_nodes.size()),
      m_generation(0),
      m_node_data(m_number_of_nodes),
      m_last_write_per_location(Context::instance().get_position_handler().tag_end(), topological_ordered_actions.iend()),
      m_topological_ordered_actions(topological_ordered_actions),
      m_location_id_to_rf_location(Context::instance().get_position_handler().tag_end())
{
  // The opsem subgraph is the first subgraph, and always present.
  RFLocation index{m_current_subgraphs.ibegin()};
  push(*this);
  // Initialize m_location_id_to_rf_location.
  for (auto&& location_subgraphs : read_from_location_subgraphs_vector)
  {
    Dout(dc::notice, location_subgraphs.location() << ':');
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
      Dout(dc::notice, subgraph);
    }
  }
}

boolean::Expression const& ReadFromGraph::loop_detected()
{
  static int count = 0;
  DoutEntering(dc::notice, "ReadFromGraph::loop_detected() " << ++count);

  // Node m_topological_ordered_actions.ibegin() (index 0) is the starting node of the program.
  SequenceNumber const begin_node{m_topological_ordered_actions.ibegin()};

  // If no loops are detected when starting from that node then the program
  // has no loops, because every node should be reachable from begin_node.
  // Therefore it is enough to only test this first node.

  // ALL nodes should be unvisited at this point, so also begin_node.
  ASSERT(is_unvisited(begin_node));

  // Initialize the condition under which a loop is found to zero.
  m_loop_condition = false;

  if (dfs(begin_node))
    Dout(dc::notice, "Found cycle under condition " << m_loop_condition);

  // Prepare for next call to loop_detected().
  reset();

  return m_loop_condition;
}

// Returns false when no loop was detected starting from node n (and m_node_data[n].m_path_condition_per_loop_event is empty).
// Otherwise returns true and m_node_data[n].m_path_condition_per_loop_event
// contains the non-zero condition(s) under which one or more loop events exist.
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
// return true and m_node_data[4].m_path_condition_per_loop_event will contain
// the pair {LoopEvent(causal_loop, 1), E} (lets abbreviate this to {1,E}).
// Therefore, the call to dfs(6) will return true and cause
// m_node_data[6].m_path_condition_per_loop_event to contain {1,HE},
// the call to dfs(5) will return true and cause
// m_node_data[5].m_path_condition_per_loop_event to contain {1,GHE},
// and the call to dfs(3) will return true and cause
// m_node_data[3].m_path_condition_per_loop_event to contain {1,DE + FGHE}.
// The call to dfs(1) will return true and cause
// m_node_data[1].m_path_condition_per_loop_event to contain {1, BCDE + BCFGHE}
// and m_loop_condition to be updated to BCDE + BCFGHE.
// Finally, the call to dfs(0) will return true and but cause
// m_node_data[0].m_path_condition_per_loop_event to remain empty because
// it doesn't contain any still actual loop events. The final result (m_loop_condition)
// is therefore BCDE + BCFGHE.
//
// Note that if dfs(4) is called before dfs(5), so that
// m_node_data[4].m_path_condition_per_loop_event is already contains {1,E}; then
// the call to dfs(6) will short circuit the calculation done by dfs(4) and not
// call dfs(4) again because node 1 is still be in the 'followed' state: the
// {1,E} can just be used directly to produce HE. Likewise if the children of
// node 3 are called in the opposite order. But if there would be an edge from
// node 0 to say node 5, then that is considered to be a dead cycle because
// the detected cycle that node 5 is part of "ends" at node 1 which is no longer
// marked as being 'followed' (or rather, the LoopEvent(causal_loop, 1) will
// no longer return that it is "actual").
//
// The algorithm used here, that detects under which condition a cycle will
// exist when each edge is "weighted" (with a boolean expression in this case),
// was designed by myself (Carlo Wood) in November/December 2017.
//
bool ReadFromGraph::dfs(SequenceNumber n, int current_memory_location)
{
  DoutEntering(dc::notice, "ReadFromGraph::dfs(" << n << ", " << current_memory_location << "): following children of node " << n);

  // n equals Action::m_sequence_number, the index for m_topological_ordered_actions.
  bool const is_write = m_topological_ordered_actions[n]->is_write();
  bool const is_read = m_topological_ordered_actions[n]->is_read();
  SequenceNumber previous_write;
  ast::tag location;

  if (is_read || is_write)
  {
    location = m_topological_ordered_actions[n]->tag();
    if (is_read)
    {
      DirectedSubgraph const* subgraph = m_current_subgraphs[m_location_id_to_rf_location[location.id]];
      //subgraph->rf_heads(n);
      SequenceNumber read_from_node{m_topological_ordered_actions.ibegin()}; // FIXME
      if (is_followed(read_from_node) && read_from_node != m_last_write_per_location[location.id])
      {
        Dout(dc::warning, "*** *** *** node " << n << " reads from node " << read_from_node <<
            " which was hidden by the write by node " << m_last_write_per_location[location.id]);
      }
    }
    if (is_write)
    {
      // Keep track of when was the last time we wrote to a memory location.
      previous_write = m_last_write_per_location[location.id];
      m_last_write_per_location[location.id] = n;
    }
  }

  set_followed(n);
  int memory_location = 0;
  for (DirectedSubgraph const* subgraph : m_current_subgraphs)
  {
    for (DirectedEdge const& directed_edge : subgraph->tails(n))
    {
      SequenceNumber const child = directed_edge.sequence_number();
      Dout(dc::notice, "Following edge to child " << child);
      if (directed_edge.is_rf_not_release_acquire())
      {
        // When memory_location == 0 this is a sb or asw edge, not an rf edge.
        ASSERT(memory_location > 0);
        Dout(dc::notice, "  (which is a rf edge of memory location '" << memory_location << "' that is not a release-acquire!)");
        if (current_memory_location > 0 && current_memory_location != memory_location)
        {
          Dout(dc::notice, "  Continuing because we already encountered a non-rel-acq Read-From edge for memory location " << current_memory_location << "!");
          continue;
        }
        current_memory_location = memory_location;
      }
      if (is_dead_end(child) || is_dead_cycle(child))
      {
        Dout(dc::notice, "  continuing because that node is dead.");
        continue;
      }
      if (is_followed(child))
      {
        Dout(dc::notice, "  cycle detected! Marking node " << child << " as end_point.");
        m_node_data[n].m_path_condition_per_loop_event.add_new(LoopEvent(causal_loop, child), directed_edge.condition().copy());
        Dout(dc::notice, "  path_condition_per_loop_condition is now " << m_node_data[n].m_path_condition_per_loop_event);
      }
      else
      {
        // If is_cycle returns true then we have a current cycle because the cycle isn't dead when we get here.
        if (is_cycle(child) || dfs(child, current_memory_location))
        {
          Dout(dc::notice, "  cycle detected behind child " << child << " of node " << n <<
              " with path_condition_per_loop_event: " << m_node_data[child].m_path_condition_per_loop_event);
          m_node_data[n].m_path_condition_per_loop_event.add_new(m_node_data[child].m_path_condition_per_loop_event, directed_edge.condition(), this);
          Dout(dc::notice, "  path_condition_per_loop_event is now " << m_node_data[n].m_path_condition_per_loop_event);
        }
      }
    }
    // When memory_location is larger than 0 the edges being followed
    // are Read-From edges of a certain memory location; different values
    // of memory_location mean different memory locations.
    ++memory_location;
  }
  Dout(dc::notice, "Done following children of node " << n);
  bool loop_detected = !m_node_data[n].m_path_condition_per_loop_event.empty();
  if (loop_detected)
  {
    set_cycle(n);
    boolean::Expression loop_condition = m_node_data[n].m_path_condition_per_loop_event.current_loop_condition(n);
    if (!loop_condition.is_zero())
    {
      Dout(dc::notice, "  loop(s) detected: " << m_node_data[n].m_path_condition_per_loop_event << ", under condition " << loop_condition << '.');
      m_loop_condition += loop_condition;
      Dout(dc::notice, "m_loop_condition is now: " << m_loop_condition << '.');
    }
  }
  else
    set_dead_end(n);

  if (is_write)
  {
    m_last_write_per_location[location.id] = previous_write;
  }

  return loop_detected;
}
