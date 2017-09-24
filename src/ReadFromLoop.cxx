#include "sys.h"
#include "ReadFromLoop.h"
#include "FollowVisitedOpsemHeads.h"
#include "FilterLocation.h"
#include "ReadFromLoopsPerLocation.h"
#include "Action.inl"

// Returns true when condition was moved (to m_write_actions or m_queued_actions).
bool ReadFromLoop::store_write(Action* write_action, boolean::Expression&& condition, boolean::Expression& found_write, bool queue)
{
  // The actual condition under which a Read-From edge from this write node exists.
  boolean::Expression rf_exists{condition * write_action->exists().as_product()};
  Dout(dc::notice, "Actual condition under which a path from " <<
      write_action->name() << " --> " << m_read_action->name() <<
      " exists: " << rf_exists);
  // The condition under which two parallel edges would exist at the same time.
  boolean::Expression parallel{found_write.times(rf_exists)};
  bool independent = parallel.is_zero();
  if (independent)
  {
    Dout(dc::notice, "Storing write because found_write = " <<
        found_write << " and (" << found_write << ") * " <<
        rf_exists << " = 0.");
    found_write += rf_exists;                                           // Update the condition under which we found writes.
    Dout(dc::notice, "Updated found_write to " << found_write);
  }
  else
  {
    Dout(dc::notice, (queue ? "Queuing" : "Keeping") << " write action " << *write_action <<
        (queue ? "" : " queued") << " because under non-zero condition " << parallel <<
        " it happens at the same time as a write that we already found.");
    if (queue)
      m_queued_actions.emplace_back(write_action, std::move(condition));
    return queue;
  }
  write_actions_type::iterator write_action_iter = m_write_actions.find(write_action);
  if (write_action_iter == m_write_actions.end())
    m_write_actions.emplace(write_action, std::move(condition));        // Store the next write action that was found.
  else
  {
    Dout(dc::notice, "This write was found before with condition " << write_action_iter->second);
    write_action_iter->second += condition;                             // Update its condition if it already existed.
    Dout(dc::continued, "; condition now " << m_write_actions[write_action]);
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
    boolean::Expression is_full_visited;
    LambdaData(ReadFromLoopsPerLocation const& read_from_loops_per_location_) :
      found_write(false),
      at_end_of_loop(true),
      read_from_loops_per_location(read_from_loops_per_location_),
      is_full_visited(true) { }
  };
  LambdaData data(read_from_loops_per_location);
  if (m_first_iteration)
  {
    ASSERT(m_queued_actions.empty());
    // Look for the last write (if any) on the same thread.
    FollowVisitedOpsemHeads follow_visited_opsem_heads(visited_generation, data.is_full_visited);
    FilterLocation filter_location(m_read_action->location());
    m_read_action->for_actions(
        follow_visited_opsem_heads,       // Search backwards in the same thread and/or joined child threads, or
                                          // parent threads when all other child threads visited their asw creation edge.
        filter_location,                  // for accesses to the same memory location.
        [this, &data](Action* action, boolean::Product const& path_condition)  // if_found
        {
          Dout(dc::notice, "Calling if_found() with is_full_visited = " << data.is_full_visited);
          if (action->is_write())
          {
            Dout(dc::notice|continued_cf, "Found write " << *action << " if " << path_condition);
            store_write(action, boolean::Expression{path_condition}, data.found_write, true);
            Dout(dc::finish, ".");
            data.at_end_of_loop = false;
          }
          else
          {
            return false;
            // Does this ever happen?
            ASSERT(action->is_read());
            Dout(dc::notice, "Found read " << *action << " if " << path_condition << "; adding Read-Froms from where that read reads from:");
            for (auto const& write_action_condition_pair : data.read_from_loops_per_location.get_read_from_loop_of(action).m_write_actions)
            {
              boolean::Expression condition{write_action_condition_pair.second * path_condition};
              Dout(dc::notice|continued_cf, "... write " << *write_action_condition_pair.first << " if " << condition);
              store_write(write_action_condition_pair.first, std::move(condition), data.found_write, true);
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
      bool stored = store_write(queued_action->first, std::move(queued_action->second), data.found_write, false);
      Dout(dc::finish, ".");
      if (stored)
        queued_action = m_queued_actions.erase(queued_action);
      else
        ++queued_action;
    }
  }
  return !data.at_end_of_loop;
}
