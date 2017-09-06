#include "sys.h"
#include "Action.h"
#include "Context.h"

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
  os << "{id:" << action.m_id << ", thread:" << action.m_thread->id() << ", location:" << *action.m_location << ", kind:" << action.kind() << '}';
  return os;
}

Action::Action(id_type next_node_id, ThreadPtr const& thread, ast::tag variable) : m_id(next_node_id), m_thread(thread)
{
#ifdef CWDEBUG
  locations_type const& locations{Context::instance().locations()};
  m_location = std::find_if(locations.begin(), locations.end(), [variable](Location const& loc) { return loc == variable; });
  ASSERT(m_location != locations.end());
#endif
}
