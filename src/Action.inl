#pragma once

#include <libcwd/type_info.h>

#ifdef CWDEBUG
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
      Dout(dc::for_action, "Following edge from " << *this << " to " << *other_node);
      // Is this the type of action that we're looking for?
      if (filter(*other_node))
      {
        Dout(dc::for_action, "Calling if_found(" << *other_node << ")");
        if (if_found(other_node))
        {
          Dout(dc::for_action, "Continuing with next end_point, if any...");
          continue;
        }
      }
#ifdef CWDEBUG
      else
        Dout(dc::for_action, "Skipping node " << *other_node << " because it was filtered.");
#endif
      Dout(dc::for_action, "Going deeper...");
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
    if (follow(end_point))
    {
      // The condition under which we can follow this path up to and including this edge.
      boolean::Product new_path_condition{path_condition};
      new_path_condition *= end_point.edge()->condition();
      // The node that we find on the other end of the edge.
      Action* other_node{end_point.other_node()};
      Dout(dc::for_action, "Following the " << end_point.edge_type() << " edge from node " << name() <<
          " to node " << other_node->name() << "; condition is now " << new_path_condition);
      // Is this the type of action that we're looking for?
      if (filter(*other_node))
      {
        Dout(dc::for_action, "Calling if_found(" << *other_node << ", " << new_path_condition << ")");
        if (if_found(other_node, new_path_condition))
        {
          Dout(dc::for_action, "Continuing with next end_point, if any...");
          continue;
        }
      }
#ifdef CWDEBUG
      else
        Dout(dc::for_action, "Skipping node " << *other_node << " because it was filtered.");
#endif
      Dout(dc::for_action, "Going deeper...");
      other_node->for_actions(follow, filter, if_found, new_path_condition);
    }
}
