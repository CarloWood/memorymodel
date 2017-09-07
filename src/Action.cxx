#include "sys.h"
#include "Action.h"
#include "Context.h"
#include "utils/macros.h"

//static
char const* Action::action_kind_str(Kind kind)
{
  switch (kind)
  {
    case lock:
      return "lock";
    case unlock:
      return "unlock";
    case atomic_load:
      return "atomic_load";
    case atomic_store:
      return "atomic_store";
    case atomic_rmw:
      return "atomic_rmw";
    case non_atomic_read:
      return "na_read";
    case non_atomic_write:
      return "na_write";
    case fence:
      return "fence";
  }
  return "UNKNOWN Action::Kind";
}

std::ostream& operator<<(std::ostream& os, Action const& action)
{
  os << "{node:" << action.name() << ", thread:" << action.m_thread->id() << ", location:" << *action.m_location << ", kind:" << action.kind() << '}';
  return os;
}

Action::Action(id_type next_node_id, ThreadPtr const& thread, ast::tag variable) : m_id(next_node_id), m_thread(thread), m_exists(true)
{
#ifdef CWDEBUG
  locations_type const& locations{Context::instance().locations()};
  m_location = std::find_if(locations.begin(), locations.end(), [variable](Location const& loc) { return loc == variable; });
  ASSERT(m_location != locations.end());
#endif
}

void Action::add_end_point(Edge* edge, EndPointType type, NodeBase const* other_node, bool edge_owner) const
{
  DoutEntering(*dc::edge[type], "Action::add_end_point(" << *edge << ", " << type << ", " << *other_node << ", " << edge_owner << ") [this = " << *this << "]");
  m_end_points.emplace_back(edge, type, other_node, edge_owner);
  // Only test this for opsem edges (and even then it appears never to happen).
  if (edge->is_opsem())
  {
    end_points_type::iterator begin = m_end_points.begin();
    end_points_type::iterator last = m_end_points.end();
    end_points_type::iterator iter = --last;      // Point to just added element.
    while (iter != begin)
      if (AI_UNLIKELY(*last == *--iter))          // Is added end point equal to already existing one?
      {
        // End point already existed.
        m_end_points.pop_back();
        ASSERT(false); // FIXME: does this ever happen?
      }
  }
  // End point did not already exist.
}

void Action::add_edge_to(EdgeType edge_type, Action const& head) const
{
}
