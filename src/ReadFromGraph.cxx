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

  // ALL nodes should be white at this point, so also node 0.
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

// Returns false when no loop was detected starting from node n (in which case m_loop_condition[n] is invalidated).
// Otherwise returns true and m_loop_condition[n] is set to the non-zero condition under which one or more loops exist.
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
// that node 1 will always be gray; hence that dfs(4) will return true
// and set m_loop_condition[4] to E.
// Therefore, the call to dfs(6) will return true and set m_loop_condition[6] to HE,
// the call to dfs(5) will return true and set m_loop_condition[5] to GHE,
// and the call to dfs(3) will return true and set m_loop_condition[3] to DE + FGHE.
// Finally, the call to dfs(0) will return true and set m_loop_condition[0] to ABCDE + ABCFGHE.
//
bool ReadFromGraph::dfs(int n)
{
  DoutEntering(dc::notice, "ReadFromGraph::dfs(" << node_id(n) << "): following children of node " << node_id(n));
  set_followed(n);
  boolean::Expression loop_condition{false};
  int end_point = -1;
  for (DirectedSubgraph const* subgraph : m_current_subgraphs)
    for (DirectedEdge const& directed_edge : subgraph->tails(n))
    {
      int const child = directed_edge.id();
      Dout(dc::notice, "Following edge to child " << node_id(child));
      if (is_dead_end(child) || is_dead_cycle(child))
      {
        Dout(dc::notice, "  continueing because that node is dead.");
        continue;
      }
      if (is_followed(child))
      {
        Dout(dc::notice, "  cycle detected! Marking node " << node_id(child) << " as end_point.");
        end_point = child;
        loop_condition += directed_edge.condition();
        Dout(dc::notice, "  loop_condition is now " << loop_condition);
      }
      else if (is_current_cycle(child) || dfs(child))
      {
        Dout(dc::notice, "  cycle detected behind child " << node_id(child) << " of node " << node_id(n) << " with node " << node_id(m_node_data[child].m_end_point) << " as end_point.");
        end_point = m_node_data[child].m_end_point;
        loop_condition += m_node_data[child].m_loop_condition.times(directed_edge.condition());
        Dout(dc::notice, "  loop_condition is now " << loop_condition);
      }
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
