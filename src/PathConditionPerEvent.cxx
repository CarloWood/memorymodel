#include "sys.h"
#include "PathConditionPerEvent.h"
#include "ReadFromGraph.h"

// A new Event has been discovered behind an edge that exists
// when condition is true. It should be added to this object.
//
// It is possible that the event already exists, in which case the
// condition must be OR-ed with the existing condition for that
// event because in that case we're dealing with an alternative path.
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
// visit 0-1-2-3-4-5 and then discover the (causal) Event "1", which
// subsequently will end up in the PathConditionPerEvent of node 4
// (with condition A).
// At that point it would follow the edge from 4 to 1 and rediscover the
// same Event "1" under condition B. From the point of view of node 4
// Event "1" then exists when A + B.
//
void PathConditionPerEvent::add_new(Event const& event, boolean::Expression&& condition)
{
  for (auto iter = m_map.begin(); iter != m_map.end(); ++iter)
  {
    if (iter->first == event)
    {
      iter->second += condition;
      return;
    }
  }
  if (!condition.is_zero())
    m_map.emplace_back(event, std::move(condition));
}

// We discovered a node behind an edge that exists when condition is true
// that was processed before (by dfs) and has associated the events of
// path_condition_per_event (if any) each with its accompanying path
// conditions. We need to add this to this object.
//
// As above, if a Event already exists then we need to update its
// path condition by OR-ing it with the newly discovered condition.
//
void PathConditionPerEvent::add_new(
    PathConditionPerEvent const& path_condition_per_event,
    boolean::Expression const& condition,
    ReadFromGraph const* read_from_graph)
{
  for (auto&& event_path_condition_pair : path_condition_per_event.m_map)
  {
    Event const& event(event_path_condition_pair.first);
    // Only consider Events that are still relevant for the current search path.
    if (event.is_relevant(*read_from_graph))
      add_new(event, event_path_condition_pair.second.times(condition));
  }
}

bool PathConditionPerEvent::contains_relevant_event(ReadFromGraph const* read_from_graph) const
{
  for (auto&& event_path_condition_pair : m_map)
    if (event_path_condition_pair.first.is_relevant(*read_from_graph))
      return true;
  return false;
}

// Remove events that have current_node as end_point,
// and return the condition under which these events invalidate the graph.
boolean::Expression PathConditionPerEvent::current_loop_condition(SequenceNumber current_node)
{
  boolean::Expression invalid_condition{false};
  size_t len = m_map.size();
  for (size_t i = 0; i < len; ++i)
  {
    Event const& event{m_map[i].first};
    if (event.end_point() == current_node)
    {
      // Read events that aren't hidden don't make an execution invalid, of course.
      if (!(event.type() == reads_from && !event.is_hidden()))
        invalid_condition += m_map[i].second;
      if (i != --len)
        m_map[i] = std::move(m_map[len]);
      m_map.pop_back();
      --i;
    }
  }
  return invalid_condition;
}

void PathConditionPerEvent::set_hidden(ast::tag location, TopologicalOrderedActions const& topological_ordered_actions)
{
  size_t len = m_map.size();
  for (size_t i = 0; i < len; ++i)
  {
    Event& event{m_map[i].first};
    if (event.type() == reads_from && !event.is_hidden() && topological_ordered_actions[event.end_point()]->tag() == location)
    {
      for (size_t j = 0; j < len; ++j)
      {
        Event& event_j{m_map[j].first};
        if (event_j.type() == reads_from && event_j.is_hidden() && event_j.end_point() == event.end_point())
        {
          size_t s = std::min(i, j);
          size_t l = i + j - s;
          m_map[s].second += m_map[l].second;
          if (l != len - 1)
          {
            m_map[l] = std::move(m_map[--len]);
            m_map.pop_back();
          }
          if (l == i)
            --i;
          break;
        }
      }
      event.set_hidden();
    }
  }
}

std::ostream& operator<<(std::ostream& os, PathConditionPerEvent const& path_condition_per_event)
{
  os << '{';
  char const* separator = "";
  for (auto&& event_path_condition_pair : path_condition_per_event.m_map)
  {
    os << separator << event_path_condition_pair;
    separator = ", ";
  }
  return os << '}';
}
