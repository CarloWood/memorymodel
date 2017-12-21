#include "sys.h"
#include "Properties.h"
#include "ReadFromGraph.h"

// A new Quirk has been discovered behind an edge that exists
// when condition is true. It should be added to this object.
//
// It is possible that the quirk already exists, in which case the
// condition must be OR-ed with the existing condition for that
// quirk because in that case we're dealing with an alternative path.
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
// visit 0-1-2-3-4-5 and then discover the (causal) Quirk "1", which
// subsequently will end up in the Properties of node 4
// (with condition A).
// At that point it would follow the edge from 4 to 1 and rediscover the
// same Quirk "1" under condition B. From the point of view of node 4
// Quirk "1" then exists when A + B.
//
void Properties::add_new(Quirk const& quirk, boolean::Expression&& condition)
{
  for (auto iter = m_map.begin(); iter != m_map.end(); ++iter)
  {
    if (iter->first == quirk)
    {
      iter->second += condition;
      return;
    }
  }
  if (!condition.is_zero())
    m_map.emplace_back(quirk, std::move(condition));
}

// We discovered a node behind an edge that exists when condition is true
// that was processed before (by dfs) and has associated the quirks of
// properties (if any) each with its accompanying path
// conditions. We need to add this to this object.
//
// As above, if a Quirk already exists then we need to update its
// path condition by OR-ing it with the newly discovered condition.
//
void Properties::add_new(
    Properties const& properties,
    boolean::Expression const& condition,
    ReadFromGraph const* read_from_graph)
{
  for (auto&& quirk_path_condition_pair : properties.m_map)
  {
    Quirk const& quirk(quirk_path_condition_pair.first);
    // Only consider Quirks that are still relevant for the current search path.
    if (quirk.is_relevant(*read_from_graph))
      add_new(quirk, quirk_path_condition_pair.second.times(condition));
  }
}

bool Properties::contains_relevant_quirk(ReadFromGraph const* read_from_graph) const
{
  for (auto&& quirk_path_condition_pair : m_map)
    if (quirk_path_condition_pair.first.is_relevant(*read_from_graph))
      return true;
  return false;
}

// Remove quirks that have current_node as end_point,
// and return the condition under which these quirks invalidate the graph.
boolean::Expression Properties::current_loop_condition(SequenceNumber current_node)
{
  boolean::Expression invalid_condition{false};
  size_t len = m_map.size();
  for (size_t i = 0; i < len; ++i)
  {
    Quirk const& quirk{m_map[i].first};
    if (quirk.end_point() == current_node)
    {
      // Read quirks that aren't hidden don't make an execution invalid, of course.
      if (!(quirk.type() == reads_from && !quirk.is_hidden()))
        invalid_condition += m_map[i].second;
      if (i != --len)
        m_map[i] = std::move(m_map[len]);
      m_map.pop_back();
      --i;
    }
  }
  return invalid_condition;
}

void Properties::set_hidden(ast::tag location, TopologicalOrderedActions const& topological_ordered_actions)
{
  size_t len = m_map.size();
  for (size_t i = 0; i < len; ++i)
  {
    Quirk& quirk{m_map[i].first};
    if (quirk.type() == reads_from && !quirk.is_hidden() && topological_ordered_actions[quirk.end_point()]->tag() == location)
    {
      for (size_t j = 0; j < len; ++j)
      {
        Quirk& quirk_j{m_map[j].first};
        if (quirk_j.type() == reads_from && quirk_j.is_hidden() && quirk_j.end_point() == quirk.end_point())
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
      quirk.set_hidden();
    }
  }
}

std::ostream& operator<<(std::ostream& os, Properties const& properties)
{
  os << '{';
  char const* separator = "";
  for (auto&& quirk_path_condition_pair : properties.m_map)
  {
    os << separator << quirk_path_condition_pair;
    separator = ", ";
  }
  return os << '}';
}
