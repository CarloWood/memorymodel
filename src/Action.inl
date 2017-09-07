#pragma once

template<class FOLLOW, class FILTER>
void Action::for_actions_no_condition(
  FOLLOW follow,
  FILTER filter,
  std::function<bool(Action const&)> const& if_found) const
{
  for (auto&& end_point : m_end_points)
    if (follow(end_point))
    {
      Action const& other_node{*end_point.other_node()};
      if (filter(other_node) && if_found(other_node))
        continue;
      other_node.for_actions_no_condition(follow, filter, if_found);
    }
}

template<class FOLLOW, class FILTER>
boolean::Expression Action::for_actions(
  FOLLOW follow,
  FILTER filter,
  std::function<bool(Action const&)> const& if_found) const
{
  boolean::Expression found(false);
  for (auto&& end_point : m_end_points)
    if (follow(end_point))
    {
      // The condition under which we CAN follow this edge.
      boolean::Product const& edge_condition{end_point.edge()->condition().boolean_product()};
      // The node that we find on the other end of the edge.
      Action const& other_node{*end_point.other_node()};
      // Is this the type of action that we're looking for?
      if (filter(other_node))
      {
        // We found this node when 1) it exists, 2) the edge towards it exists.
        // If we are following Sequenced-Before edges downwards, then the edge
        // condition is already part of the value returned by exists, but this
        // does not harm since A * A = A.
        found += other_node.exists() * edge_condition;
        // Call if_found with the node that we found.
        if (!if_found(other_node))      // Only follow edges beyond this node when if_found returns false.
          other_node.for_actions_no_condition(follow, filter, if_found);
        continue;
      }
      found += other_node.for_actions(follow, filter, if_found) * edge_condition;
    }
  return found;
}
