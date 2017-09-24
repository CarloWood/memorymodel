#pragma once

#ifdef CWDEBUG
#include <libcwd/type_info.h>
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct for_action;
NAMESPACE_DEBUG_CHANNELS_END
#endif

template<class FOLLOW, class FILTER>
void Action::for_actions_no_condition(
  FOLLOW follow,
  FILTER filter,
  std::function<bool(Action*)> const& if_found) const
{
  DoutEntering(dc::for_action, "Action::for_actions_no_condition<" <<
      type_info_of<FOLLOW>().demangled_name() << ", " <<
      type_info_of<FILTER>().demangled_name() << ">(...) [this = " << *this << "].");
  for (auto&& end_point : m_end_points)
    if (follow(end_point))
    {
      // The node that we find on the other end of the edge.
      Action* other_node{end_point.other_node()};
      Dout(dc::for_action, "Following edge from " << name() << " to " << other_node->name() << '.');
      // Is this the type of action that we're looking for?
      if (filter(*other_node))
      {
        Dout(dc::for_action, "Calling if_found(" << *other_node << ")");
        DebugMarkDownRight;
        if (if_found(other_node))
        {
          Dout(dc::for_action, "Continuing with next end_point of node " << name() << ", if any...");
          continue;
        }
      }
      Dout(dc::for_action, "Following edges of node " << other_node->name() << "...");
      other_node->for_actions_no_condition(follow, filter, if_found);
    }
}

template<class FOLLOW, class FILTER>
void Action::for_actions(
  FOLLOW follow,
  FILTER filter,
  std::function<bool(Action*, boolean::Product const&)> const& if_found,
  boolean::Product const& path_condition) const
{
  DoutEntering(dc::for_action, "Action::for_actions<" <<
      type_info_of<FOLLOW>().demangled_name() << ", " <<
      type_info_of<FILTER>().demangled_name() << ">(..., " << path_condition << ") [this = " << *this << "].");
  for (auto&& end_point : m_end_points)
    if (follow(end_point, path_condition))
    {
      // The condition under which we can follow this path up to and including this edge.
      boolean::Product new_path_condition{path_condition};
      new_path_condition *= follow.is_fully_visited().as_product();
      new_path_condition *= end_point.edge()->condition().as_product();
      // The node that we find on the other end of the edge.
      Action* other_node{end_point.other_node()};
#ifdef CWDEBUG
      Dout(dc::for_action|continued_cf, "Following the " << end_point.edge_type() << " edge from node " << name() <<
          " to node " << other_node->name());
      if (!end_point.edge()->condition().is_one())
        Dout(dc::continued, "; condition is now " << new_path_condition);
      Dout(dc::finish, ".");
#endif
      // Is this the type of action that we're looking for?
      if (filter(*other_node))
      {
        Dout(dc::for_action, "Calling if_found(" << *other_node << ", " << new_path_condition << ")");
        DebugMarkDownRight;
        if (if_found(other_node, new_path_condition))
        {
          Dout(dc::for_action, "Continuing with next end_point of node " << name() << ", if any...");
          continue;
        }
      }
      Dout(dc::for_action, "Following edges of node " << other_node->name() << "...");
      other_node->for_actions(follow, filter, if_found, new_path_condition);
    }
}
