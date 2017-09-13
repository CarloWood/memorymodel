#pragma once

#include <bitset>

class Action;

class ActionSet
{
  static constexpr std::size_t max_actions = 64;
  std::bitset<max_actions> m_actions; // Action

 public:
  inline void add(Action const& action);
  inline void remove(Action const& action);

  // Union.
  void add(ActionSet const& action_set) { m_actions |= action_set.m_actions; }

  // Return true if action is element of the set.
  inline bool includes(Action const& action) const;
};
