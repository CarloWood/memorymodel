#include "sys.h"
#include "debug.h"
#include "ReadFromGraph.h"
#include "Action.h"
#include "Context.h"
#include "Propagator.h"
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
  DoutEntering(dc::notice, "ReadFromGraph::loop_detected()");

  // Node 0 is the starting node of the program.
  m_current_node.set_to_zero();

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
bool ReadFromGraph::dfs()
{
  DoutEntering(dc::readfrom, "ReadFromGraph::dfs() for node " << m_current_node << '.');

  // Depth-First search only visits each node once.
  ASSERT(is_unvisited(m_current_node));

  // Current node data.
  Properties& current_properties{m_node_data[m_current_node].m_properties};

  // Reset any old values in m_properties (from a previous generation).
  current_properties.reset();

  // m_current_node equals Action::m_sequence_number, the index for m_topological_ordered_actions.
  Action* current_action = m_topological_ordered_actions[m_current_node];
  ast::tag location = current_action->tag();
  RFLocation current_location{m_location_id_to_rf_location[location.id]};
  bool const have_location_subgraph = !current_location.undefined() && current_location < m_current_subgraphs.iend();
  bool const is_write = have_location_subgraph && current_action->is_write();
  bool const is_read = have_location_subgraph && current_action->is_read();

  if (is_read)
  {
    DirectedSubgraph const* subgraph = m_current_subgraphs[current_location];
    for (auto incoming_read_from_edge = subgraph->edges(m_current_node).begin_incoming();
         incoming_read_from_edge != subgraph->edges(m_current_node).end_incoming();
         ++incoming_read_from_edge)
    {
      SequenceNumber write_node{incoming_read_from_edge->tail_sequence_number()};
      Dout(dc::readfrom, "Node " << m_current_node << " reads from node " << write_node);
      current_properties.add(Property(reads_from, write_node, incoming_read_from_edge->condition()));
      Dout(dc::readfrom, "  " << m_current_node << ".properties is now " << current_properties);
    }
  }

  set_followed(m_current_node);

  // Run over all relevant subgraphs (two of them).
  DirectedSubgraph const* subgraph = m_current_subgraphs.front();
  for (int opsem_or_rf = 0; opsem_or_rf < 2; ++opsem_or_rf)     // 0: edge is opsem, 1: edge is rf.
  {
    // Run over all outgoing directed edges of the current node.
    for (auto directed_edge = subgraph->edges(m_current_node).begin_outgoing(); directed_edge != subgraph->edges(m_current_node).end_outgoing(); ++directed_edge)
    {
      SequenceNumber const child = directed_edge->head_sequence_number();
      Dout(dc::readfrom, "Following edge to child " << child);

      if (is_processed(child) || is_visited_but_not_relevant(child))
      {
        Dout(dc::readfrom, "  continuing because that node is already processed.");
        continue;
      }
      if (!is_followed(child))
      {
        // If is_visited returns true then we have properties to be processed,
        // because if there were no relevant properties then we wouldn't get here.
        bool have_properties = is_visited(child);
        if (!have_properties)
        {
          // Call dfs recursively.
          SequenceNumber current_node{m_current_node};
          m_current_node = child;
          have_properties = dfs();
          m_current_node = current_node;
        }
        if (!have_properties)
          continue;
      }

      // If we are following an edge that is a ReadFrom then obviously the current node needs to be a write.
      ASSERT(!opsem_or_rf || is_write);

      // We fould an edge that either ends in a (child) node that
      // is currently being followed, or that was visited before
      // and has unprocessed properties. In the latter case the
      // properties of child need converted using a Propagator and
      // merged into the properties of the current node.
      // In the former case we create a new causal_loop property,
      // which also has to be converted before it can be merged
      // into our own properties.
      //
      //           opsem or rf              (opsem being sb or asw).
      //    * ----------------------> *
      //    ^                         ^
      //    |                         |
      // current_*                  child*
      //
      Propagator propagator(
          current_action, m_current_node, current_location, is_write,
          m_topological_ordered_actions[child], child, opsem_or_rf,
          directed_edge->condition());

      if (is_followed(child))
      {
        Dout(dc::readfrom, "  possible causal loop detected. Marking node " << child << " as end_point.");
        // If the edge is a Read-From edge and the child is a Read acquire but
        // our current node is not a Write release then we need to wrap the
        // causal_loop Property in a release_sequence Property.
        bool is_release_sequence = propagator.rf_acq_but_not_rel();
        // Create a new causal_loop/release_sequence Property. Immediately give it the condition
        // under which our edge exists because convert() does not alter the condition
        // of Property objects, so we have to do that ourselves.
        Property new_property(is_release_sequence ? release_sequence : causal_loop, child, directed_edge->condition());
        if (is_release_sequence)
        {
          // In this case the new causal loop Property needs to be both, wrapped and added.
          Property cl_property(causal_loop, child, true);
          new_property.wrap(cl_property);
          if (cl_property.convert(propagator))
            current_properties.add(std::move(cl_property));
        }
        if (new_property.convert(propagator))
          current_properties.add(std::move(new_property));
        Dout(dc::readfrom, "  " << m_current_node << ".properties is now " << current_properties);
      }
      else
      {
        Dout(dc::readfrom, "  merging properties of child " << child << " [" << m_node_data[child].m_properties <<
            "] into node " << m_current_node << " [" << current_properties << "].");
        // Propagate the properties from child to the current node and merge them.
        current_properties.merge(m_node_data[child].m_properties, std::move(propagator), this);
        Dout(dc::readfrom, "  " << m_current_node << ".properties is now " << current_properties);
      }
    }
    // Next process the rf edges of the current node, if any.
    if (!have_location_subgraph)
      break;
    subgraph = m_current_subgraphs[current_location];
  }
  Dout(dc::readfrom, "Done following children of node " << m_current_node);
  bool have_properties = !current_properties.empty();
  if (have_properties)
  {
    set_visited(m_current_node);
    boolean::Expression loop_condition = current_properties.current_loop_condition(this);
    if (!loop_condition.is_zero())
    {
      Dout(dc::notice, "Violation(s) detected involving end point " << m_current_node << ", under condition " << loop_condition << '.');
      m_loop_condition += loop_condition;
      Dout(dc::readfrom, "m_loop_condition is now: " << m_loop_condition << '.');
    }
  }
  else
    set_processed(m_current_node);

  return have_properties;
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct readfrom("READFROM");
NAMESPACE_DEBUG_CHANNELS_END
#endif
