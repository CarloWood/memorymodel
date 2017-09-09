#pragma once

template<class FOLLOW, class FILTER>
void Action::for_actions_no_condition(
  FOLLOW follow,
  FILTER filter,
  std::function<bool(Action*)> const& if_found) const
{
  for (auto&& end_point : m_end_points)
    if (follow(end_point))
    {
      // The node that we find on the other end of the edge.
      Action* other_node{end_point.other_node()};
      // Is this the type of action that we're looking for?
      if (filter(*other_node) && if_found(other_node))
        continue;
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
  DoutEntering(dc::notice, "Action::for_actions<>(..., " << path_condition << ") [this = " << *this << "].");
  for (auto&& end_point : m_end_points)
    if (follow(end_point))
    {
      // The condition under which we can follow this path up to and including this edge.
      boolean::Product new_path_condition{path_condition};
      new_path_condition *= end_point.edge()->condition().boolean_product();
      // The node that we find on the other end of the edge.
      Action* other_node{end_point.other_node()};
      Dout(dc::notice, "Following the edge to " << *other_node << "; condition is now " << new_path_condition);
      // Is this the type of action that we're looking for?
      if (filter(*other_node))
      {
        Dout(dc::notice, "Calling if_found(" << *other_node << ", " << new_path_condition << ")");
        if (if_found(other_node, new_path_condition))
        {
          Dout(dc::notice, "Continuing with next end_point, if any...");
          continue;
        }
      }
      else
        Dout(dc::notice, "Skipping node " << *other_node << " because it was filtered.");
      Dout(dc::notice, "Going deeper...");
      other_node->for_actions(follow, filter, if_found, new_path_condition);
    }
}
