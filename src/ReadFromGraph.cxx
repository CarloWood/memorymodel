#include "sys.h"
#include "debug.h"
#include "ReadFromGraph.h"
#include "Action.h"
#include "utils/MultiLoop.h"

#ifdef CWDEBUG
int node_id(int n) { return n + 1; }
#endif

boolean::Expression const& ReadFromGraph::loop_detected()
{
  static int count = 0;
  DoutEntering(dc::notice, "ReadFromGraph::loop_detected() " << ++count);

  // Node 0 is the starting node of the program.
  //
  // If no loops are detected when starting from that node then the program
  // has no loops, because every node should be reachable from node 0.
  // Therefore it is enough to only test node 0.

  // ALL nodes should be unvisited at this point, so also node 0.
  ASSERT(is_unvisited(0));

  // dfs returns false when there is no loop detected at all; otherwise
  // m_loop_condition is set to the condition under which there is a loop.
  if (!dfs(0))
    m_node_data[0].m_loop_condition = false;
  else
  {
    Dout(dc::notice, "Found cycle under condition " << m_node_data[0].m_loop_condition);
  }

  // If we return true then we reject this graph entirely because it will always have a loop.
  // Otherwise, every node must have been visited or we can't know NOT to reject this graph.
  ASSERT((m_node_data[0].m_loop_condition.is_one() ||   // There is always a loop, or
          [this]()
          {
            int n = 0;
            while (n < m_number_of_nodes && (is_dead_end(n) || is_dead_cycle(n)))
              ++n;
            return n == m_number_of_nodes;              // all nodes processed.
          }())
  );

  // Prepare for next call to loop_detected().
  reset();

  return m_node_data[0].m_loop_condition;
}

// Returns false when no loop was detected starting from node n (in which case m_node_data[n].m_loop_condition is invalidated).
// Otherwise returns true and m_node_data[n].m_loop_condition is set to the non-zero condition under which one or more loops exist.
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
// that node 1 will always be 'followed'; hence that dfs(4) will return true
// and set m_node_data[4].m_loop_condition to E.
// Therefore, the call to dfs(6) will return true and set
// m_node_data[6].m_loop_condition to HE, the call to dfs(5) will return true
// and set m_node_data[5].m_loop_condition to GHE, and the call to dfs(3) will
// return true and set m_node_data[3].m_loop_condition to DE + FGHE.
// Finally, the call to dfs(0) will return true and set m_node_data.m_loop_condition[0]
// to ABCDE + ABCFGHE.
// Note that if dfs(4) is called before dfs(5), so that m_node_data[4].m_loop_condition
// is already set to E and m_node_data[4].m_end_point to 1; then the call to
// dfs(6) will short circuit the calculation done by dfs(4) and not call dfs(4)
// again because node 1 will still be in the 'followed' state: the boolean expression
// E can just be used directly to produce HE. Likewise if the children of node 3
// are called in the opposite order. But if there would be an edge from node 0
// to say node 5, then that is considered to be a dead cycle because the detected
// cycle that node 5 is part of "ends" at node 1 which is no longer marked as
// being 'followed'.
//
// The algorithm used here, that detects under which condition a cycle will
// exist when each edge is "weighted" (with a boolean expression in this case),
// was designed by myself (Carlo Wood) in November 2017.
//
bool ReadFromGraph::dfs(int n, int current_memory_location)
{
  DoutEntering(dc::notice, "ReadFromGraph::dfs(" << node_id(n) << ", " << current_memory_location << "): following children of node " << node_id(n));
  set_followed(n);
  boolean::Expression loop_condition{false};
  int end_point = -1;
  int memory_location = 0;
  for (DirectedSubgraph const* subgraph : m_current_subgraphs)
  {
    for (DirectedEdge const& directed_edge : subgraph->tails(n))
    {
      int const child = directed_edge.id();
      Dout(dc::notice, "Following edge to child " << node_id(child));
      if (directed_edge.is_rf_not_release_acquire())
      {
        // When memory_location == 0 this is a sb or asw edge, not an rf edge.
        ASSERT(memory_location > 0);
        Dout(dc::notice, "  (which is a rf edge of memory location '" << memory_location << "' that is not an release-acquire!)");
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
        Dout(dc::notice, "  cycle detected! Marking node " << node_id(child) << " as end_point.");
        end_point = child;
        loop_condition += directed_edge.condition();
        Dout(dc::notice, "  loop_condition is now " << loop_condition);
      }
      else
      {
        if (is_current_cycle(child) || dfs(child, current_memory_location))
        {
          Dout(dc::notice, "  cycle detected behind child " << node_id(child) << " of node " << node_id(n) << " with node " << node_id(m_node_data[child].m_end_point) << " as end_point.");
          end_point = m_node_data[child].m_end_point;
          loop_condition += m_node_data[child].m_loop_condition.times(directed_edge.condition());
          Dout(dc::notice, "  loop_condition is now " << loop_condition);
        }
      }
    }
    // When memory_location is larger than 0 the edges being followed
    // are Read-From edges of a certain memory location; different values
    // of memory_location mean different memory locations.
    ++memory_location;
  }
  bool loop_detected = !loop_condition.is_zero();
  Dout(dc::notice, "Done following children of node " << node_id(n));
  if (loop_detected)
  {
    Dout(dc::notice, "  a loop is detected here, ending at node " << node_id(end_point) << ", under condition " << loop_condition);
    set_current_cycle(n, end_point, std::move(loop_condition));
  }
  else
    set_dead_end(n);
  return loop_detected;
}
