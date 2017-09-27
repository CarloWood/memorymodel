#include "sys.h"
#include "FollowVisitedOpsemHeads.h"
#include "Action.h"
#include "FilterLocation.h"
#include "Action.inl"

void FollowVisitedOpsemHeads::process_queued(std::function<bool(Action*, boolean::Product const&)> const& if_found)
{
  auto action_iter = m_queued.begin();
  while (action_iter != m_queued.end())
  {
    Action* action = *action_iter;
    m_queued.erase(action_iter);
    FilterLocation const filter_location(action->location());
    Dout(dc::notice, "Processing next queued action " << action->name() << ':');
    boolean::Expression path_condition{action->calculate_path_condition(m_visited_generation, m_read_node)};
    //action->for_actions(*this, filter_location, if_found, path_condition);
    action_iter = m_queued.begin();
  }
}

bool FollowVisitedOpsemHeads::operator()(EndPoint const& end_point, boolean::Product const& path_condition)
{
  // Only follow heads: we go upstream in the graph.
  if (end_point.type() != head || !end_point.edge()->is_opsem())
    return false;


  // Mark this edge as being visited under condition path_condition.
  Dout(dc::visited, "Visited " << end_point.edge_type() << " edge between " << end_point.other_node()->name() <<
      " and " << end_point.current_node()->name() << " under condition " << path_condition);
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
    int sequence_number = other_node->sequence_number();
    auto pos = std::find_if(m_queued.begin(), m_queued.end(), [sequence_number](Action* action){ return action->sequence_number() < sequence_number; });
    m_queued.insert(pos, other_node);
    return false;
  }
  else
  {
    m_new_path_condition = end_point.other_node()->calculate_path_condition(m_visited_generation, m_read_node);
    Dout(dc::notice, "new_path_condition = " << m_new_path_condition);
  }
  return true;
}
