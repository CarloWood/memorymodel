#include "sys.h"
#include "FollowVisitedOpsemHeads.h"
#include "Action.h"
#include "FilterWriteLocation.h"
#include "Action.inl"

void FollowVisitedOpsemHeads::process_queued(std::function<bool(Action*, boolean::Expression&&)> const& if_found)
{
  auto action_iter = m_queued.begin();
  while (action_iter != m_queued.end())
  {
    Action* action = *action_iter;
    m_queued.erase(action_iter);
    FilterWriteLocation const filter_location(m_read_node->location());
    Dout(dc::notice, "Processing next queued action " << action->name() << ':');
    boolean::Expression path_condition{action->calculate_path_condition(m_visited_generation, m_read_node)};
    action->for_actions(*this, filter_location, if_found, path_condition);
    action_iter = m_queued.begin();
  }
}

bool FollowVisitedOpsemHeads::operator()(EndPoint const& end_point, boolean::Expression& path_condition)
{
  // Only follow heads: we go upstream in the graph.
  if (end_point.type() != head || !end_point.edge()->is_opsem())
    return false;

  // Mark this edge as being visited under condition path_condition.
  Dout(dc::visited, "Visited " << end_point.other_node()->name() << " -" << edge_name(end_point.edge_type()) << "-> " << end_point.current_node()->name() <<
      " under condition " << path_condition);
  {
    DebugMarkDownRight;
    end_point.edge()->visited(m_visited_generation, path_condition);
  }
  bool is_fully_visited = end_point.other_node()->is_fully_visited(m_visited_generation, m_read_node);
  if (!is_fully_visited)
  {
    Dout(dc::visited, "Queuing " << end_point.other_node()->name() << " because it is not fully visited yet.");
    // Queue other_node into m_queued ordered such that the node with the largest sequence number comes first.
    Action* other_node = end_point.other_node();
    auto res = m_queued.insert(other_node);
    if (!res.second)
      Dout(dc::visited, "  (already there).");
    return false;
  }
  path_condition = end_point.other_node()->calculate_path_condition(m_visited_generation, m_read_node);
  if (m_queued.erase(end_point.other_node()))
    Dout(dc::visited, "  (removed " << end_point.other_node()->name() << " from queue).");
  Dout(dc::notice, "New path_condition = " << path_condition << '.');
  Debug(path_condition.sanity_check());
  return true;
}
