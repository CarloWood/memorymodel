#pragma once

#include "Action.h"
#include "debug.h"
#include "BooleanExpression.h"
#include <map>
#include <deque>

class ReadFromLoopsPerLocation;

class ReadFromLoop
{
  using write_actions_type = std::map<Action*, boolean::Expression>;
  using queued_actions_type = std::deque<std::pair<Action*, boolean::Product>>;

 private:
  Action* m_read_action;                        // The read action that we're looping over all possible write actions that it Read-Froms.
  bool m_first_iteration;                       // Set to true upon the first iteration of this loop.
  write_actions_type m_write_actions;           // More than one write might be found depending on conditions.
  queued_actions_type m_queued_actions;         // Write actions found that couldn't be processed immediately because they happen
                                                // under the same condition(s) as what we found so far.

 public:
  ReadFromLoop(Action* read_action) : m_read_action(read_action) { }
  ReadFromLoop(ReadFromLoop&& read_from_loop) :
    m_read_action(read_from_loop.m_read_action),
    m_first_iteration(read_from_loop.m_first_iteration),
    m_write_actions(std::move(read_from_loop.m_write_actions)),
    m_queued_actions(std::move(read_from_loop.m_queued_actions)) { }

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

 private:
  void store_write(Action* write_action, boolean::Product path_condition, boolean::Expression& found_write);
};
