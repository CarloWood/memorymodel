#include "sys.h"
#include "ReadFromLoop.h"
#include "FollowVisitedOpsemHeads.h"
#include "FilterLocation.h"
#include "ReadFromLoopsPerLocation.h"
#include "Action.inl"

bool ReadFromLoop::find_next_write_action(ReadFromLoopsPerLocation const& read_from_loops_per_location, int visited_generation)
{
#ifdef CWDEBUG
  DoutEntering(dc::notice, "find_next_write_action() on ReadFromLoop for read action " << *m_read_action);
  debug::Mark marker;
#endif
  if (!m_first_iteration)
  {
    Dout(dc::notice, "Exiting because m_first_iteration is false.");
    return false;
  }
  // Look for the last write (if any) on the same thread.
  FollowVisitedOpsemHeads follow_visited_opsem_heads(visited_generation);
  FilterLocation filter_location(m_read_action->location());
  bool at_end_of_loop = false;
  m_read_action->for_actions(
      follow_visited_opsem_heads,       // Search backwards in the same thread and/or joined child threads, or
                                        // parent threads when all other child threads visited their asw creation edge.
      filter_location,                  // for accesses to the same memory location.
      [this, &read_from_loops_per_location](Action* action, boolean::Product const& path_condition)  // if_found
      {
        if (action->is_write())
        {
          auto res = m_write_actions.emplace(action, path_condition);               // Store the next write action that was found.
          if (!res.second)
            res.first->second += path_condition;                                    // Update its condition if it already existed.
          Dout(dc::notice, "Found write " << *action << " if " << path_condition << "; condition now " << m_write_actions[action]);
        }
        else
        {
          // Does this ever happen?
          ASSERT(action->is_read());
          Dout(dc::notice, "Found read " << *action << " if " << path_condition << "; adding Read-Froms from where that read reads from:");
          for (auto&& write_action_condition_pair : read_from_loops_per_location.get_read_from_loop_of(action).m_write_actions)
          {
            boolean::Product condition{path_condition};
            condition *= write_action_condition_pair.second.as_product();
            auto res = m_write_actions.emplace(write_action_condition_pair.first, condition);       // Store the next write action that was found.
            if (!res.second)
              res.first->second += condition;                                  // Update its condition if it already existed.
            Dout(dc::notice, "... write " << *write_action_condition_pair.first <<
                " if " << condition << "; condition now " << m_write_actions[write_action_condition_pair.first]);
          }
        }
        return true;                                // Stop following edges once we found a sequenced before action for the same memory locaton.
      }
  );
  m_first_iteration = false;
#ifdef CWDEBUG
  // Eventually the condition of a complete path should be just the product of the indeterminates that matter.
  for (auto&& write_action_condition_pair : m_write_actions)
    ASSERT(write_action_condition_pair.second.is_product());
#endif
  return !at_end_of_loop;
}
