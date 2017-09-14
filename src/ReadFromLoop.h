#pragma once

#include "Action.h"
#include "debug.h"
#include "BooleanExpression.h"
#include <map>

class ReadFromLoopsPerLocation;

class ReadFromLoop
{
 private:
  Action* m_read_action;
  using write_actions_type = std::map<Action*, boolean::Expression>;
  write_actions_type m_write_actions;         // More than one write might be found depending on conditions.
  bool m_first_iteration;                     // Set to true upon the first iteration of this loop.

 public:
  ReadFromLoop(Action* read_action) : m_read_action(read_action) { }

  void begin()
  {
    Dout(dc::notice, "Calling begin() on ReadFromLoop for read action " << *m_read_action);
    m_write_actions.clear();
    m_first_iteration = true;
  }

  bool find_next_write_action(ReadFromLoopsPerLocation const& read_from_loops_per_location, int visited_generation);

  void delete_edge()
  {
    // Delete any edges that were added in the last call to add_edge (if any).
    for (auto&& write_action_condition_pair : m_write_actions)
      write_action_condition_pair.first->delete_edge_to(edge_rf, m_read_action);
  }

  bool add_edge()
  {
    bool new_edges = false;
    for (auto&& write_action_condition_pair : m_write_actions)
    {
      new_edges = true;
      write_action_condition_pair.first->add_edge_to(edge_rf, m_read_action, write_action_condition_pair.second.as_product());
    }
    return new_edges;
  }
};
