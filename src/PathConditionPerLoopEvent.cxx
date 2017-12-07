#include "sys.h"
#include "PathConditionPerLoopEvent.h"
#include "ReadFromGraph.h"

// A new loop event LoopEvent has been discovered behind an edge
// that exists when condition is true. It should be added to this object.
//
// It is possible that the loop event already exists, in which case the
// condition must be OR-ed with the existing condition for that event because
// in that case we're dealing with an alternative path.
//
// For example,
//
//   0       3←.
//   |       | |
//   ↓  rf:B ↓ |
//   1←------4 |
//   |↖      | |
//   | \rf:A ↓ |
//   |  `----5 |rf
//   ↓         |
//   2---------'
//
// let node 1 be a Read that reads the Write at 4 under condition B and from
// the Write at 5 under the condition A (which can only happen when AB = 0).
// Then the algorithm when doing a depth-first search starting at 0 could
// visit 0-1-2-3-4-5 and then discover the (causal) LoopEvent "1", which
// subsequently will end up in the PathConditionPerLoopEvent of node 4
// (with condition A).
// At that point it would follow the edge from 4 to 1 and rediscover the
// same LoopEvent "1" under condition B. From the point of view of node 4
// LoopEvent "1" then exists when A + B.
//
void PathConditionPerLoopEvent::add_new(LoopEvent const& loop_event, boolean::Expression&& condition)
{
  for (auto iter = m_map.begin(); iter != m_map.end(); ++iter)
  {
    if (iter->first == loop_event)
    {
      iter->second += condition;
      return;
    }
  }
  m_map.emplace_back(loop_event, std::move(condition));
}

// We discovered a node behind an edge that exists when condition is true
// that was processed before (by dfs) and has associated the loop_events of
// path_condition_per_loop_event (if any) each with its accompanying path
// conditions. We need to add this to this object.
//
// As above, if a LoopEvent already exists then we need to update its
// path condition by OR-ing it with the newly discovered condition.
//
void PathConditionPerLoopEvent::add_new(
    PathConditionPerLoopEvent const& path_condition_per_loop_event,
    boolean::Expression const& condition,
    ReadFromGraph const* read_from_graph)
{
  for (auto&& loop_event_path_condition_pair : path_condition_per_loop_event.m_map)
  {
    LoopEvent const& loop_event(loop_event_path_condition_pair.first);
    // Only consider LoopEvents that are still actual for the current search path.
    if (loop_event.is_actual(*read_from_graph))
      add_new(loop_event, loop_event_path_condition_pair.second.times(condition));
  }
}

bool PathConditionPerLoopEvent::contains_actual_loop_event(ReadFromGraph const* read_from_graph) const
{
  for (auto&& loop_event_path_condition_pair : m_map)
    if (loop_event_path_condition_pair.first.is_actual(*read_from_graph))
      return true;
  return false;
}

// Return the condition under which there is a loop ending in end_point.
boolean::Expression PathConditionPerLoopEvent::current_loop_condition(int end_point) const
{
  boolean::Expression result{false};
  for (auto&& loop_event_path_condition_pair : m_map)
    if (loop_event_path_condition_pair.first.end_point() == end_point) // FIXME: can't a LoopEvent "terminate" before we backtrack to it's 'end point'?
      result += loop_event_path_condition_pair.second;
  return result;
}

std::ostream& operator<<(std::ostream& os, PathConditionPerLoopEvent const& path_condition_per_loop_event)
{
  os << '{';
  char const* separator = "";
  for (unsigned int i = 0; i < path_condition_per_loop_event.m_map.size(); ++i)
  {
    os << separator << path_condition_per_loop_event.m_map[i];
    separator = ", ";
  }
  return os << '}';
}
