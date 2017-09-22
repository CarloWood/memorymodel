#include "sys.h"
#include "ReadFromLoop.h"
#include "FollowVisitedOpsemHeads.h"
#include "FilterLocation.h"
#include "ReadFromLoopsPerLocation.h"
#include "Action.inl"

bool ReadFromLoop::store_write(Action* write_action, boolean::Product path_condition, boolean::Expression& found_write, bool queue)
{
  auto res = m_write_actions.emplace(write_action, boolean::Expression{path_condition});     // Store the next write action that was found.
  if (!res.second)
  {
    Dout(dc::notice, "This write was found before with condition " << res.first->second);
    res.first->second += path_condition;                                // Update its condition if it already existed.
    Dout(dc::continued, "; condition now " << m_write_actions[write_action]);
  }
  // The actual condition under which this path exists.
  path_condition *= write_action->exists().as_product();
  Dout(dc::notice, "Actual condition under which path from " << write_action->name() << " --> " << m_read_action->name() << " exists: " << path_condition);
  if ((found_write * path_condition).is_zero())
  {
    Dout(dc::notice, "Keeping write because found_write = " << found_write << " and (" << found_write << ") * " << path_condition  << " = " << (found_write * path_condition) << '.');
    found_write += path_condition;                                      // Update the condition under which we found writes.
    Dout(dc::notice, "Updated found_write to " << found_write);
  }
  else
  {
    Dout(dc::notice, "Queuing write action " << *res.first->first << " because under non-zero condition " << (found_write * path_condition) <<
        " it happens at the same time as a write that we already found.");
    if (queue)
      m_queued_actions.emplace_back(res.first->first, std::move(res.first->second.as_product()));
    m_write_actions.erase(res.first);
    return false;
  }
  return true;
}

bool ReadFromLoop::find_next_write_action(ReadFromLoopsPerLocation const& read_from_loops_per_location, int visited_generation)
{
#ifdef CWDEBUG
  DoutEntering(dc::notice, "find_next_write_action() on ReadFromLoop for read action " << *m_read_action);
  debug::Mark marker;
#endif
  // g++ starts allocating memory for lambda's capturing more than 16 bytes (clang++ 24 bytes), so put all data that we need in a struct.
  struct LambdaData {
    boolean::Expression found_write;    // Boolean expression under which we found a write.
    bool at_end_of_loop;
    ReadFromLoopsPerLocation const& read_from_loops_per_location;
    LambdaData(ReadFromLoopsPerLocation const& read_from_loops_per_location_) :
      found_write(false),
      at_end_of_loop(true),
      read_from_loops_per_location(read_from_loops_per_location_) { }
  };
  LambdaData data(read_from_loops_per_location);
  if (m_first_iteration)
  {
    ASSERT(m_queued_actions.empty());
    // Look for the last write (if any) on the same thread.
    FollowVisitedOpsemHeads follow_visited_opsem_heads(visited_generation);
    FilterLocation filter_location(m_read_action->location());
    m_read_action->for_actions(
        follow_visited_opsem_heads,       // Search backwards in the same thread and/or joined child threads, or
                                          // parent threads when all other child threads visited their asw creation edge.
        filter_location,                  // for accesses to the same memory location.
        [this, &data](Action* action, boolean::Product const& path_condition)  // if_found
        {
          if (action->is_write())
          {
            Dout(dc::notice|continued_cf, "Found write " << *action << " if " << path_condition);
            store_write(action, path_condition, data.found_write, true);
            Dout(dc::finish, ".");
            data.at_end_of_loop = false;
          }
          else
          {
            return false;
            // Does this ever happen?
            ASSERT(action->is_read());
            Dout(dc::notice, "Found read " << *action << " if " << path_condition << "; adding Read-Froms from where that read reads from:");
            for (auto&& write_action_condition_pair : data.read_from_loops_per_location.get_read_from_loop_of(action).m_write_actions)
            {
              boolean::Product condition{path_condition};
              condition *= write_action_condition_pair.second.as_product();
              Dout(dc::notice|continued_cf, "... write " << *write_action_condition_pair.first << " if " << condition);
              store_write(write_action_condition_pair.first, condition, data.found_write, true);
              Dout(dc::finish, ".");
              data.at_end_of_loop = false;
            }
          }
          return true;                    // Stop following edges once we found a sequenced before action for the same memory locaton.
        }
    );
    m_first_iteration = false;
  }
  else
  {
    data.at_end_of_loop = m_queued_actions.empty();
    for (queued_actions_type::iterator queued_action = m_queued_actions.begin(); queued_action != m_queued_actions.end();)
    {
      Dout(dc::notice|continued_cf, "Found queued write " << *queued_action->first << " if " << queued_action->second);
      bool stored = store_write(queued_action->first, queued_action->second, data.found_write, false);
      Dout(dc::finish, ".");
      if (stored)
        queued_action = m_queued_actions.erase(queued_action);
      else
        ++queued_action;
    }
  }
  return !data.at_end_of_loop;
}
