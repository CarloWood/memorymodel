#pragma once

template<typename FOLLOW>
bool Action::for_actions(
  FOLLOW follow,
  std::function<bool(Action const&)> const& filter,
  std::function<bool(Action const&)> const& found) const
{
  for (auto&& end_point : m_end_points)
    if (follow(end_point))
    {
      Action const& other_node{*end_point.other_node()};
      if (filter(other_node))
        if (found(other_node))
          return true;
      if (other_node.for_actions(follow, filter, found))
        return true;
    }
  return false;
}
