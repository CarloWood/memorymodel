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
  struct ReadFromIfFoundData
  {
    boolean::Expression found_write;    // Boolean expression under which we found a write.
    bool at_end_of_loop;
    ReadFromLoopsPerLocation const& read_from_loops_per_location;
    boolean::Expression new_path_condition;

    ReadFromIfFoundData(ReadFromLoopsPerLocation const& read_from_loops_per_location_) :
      found_write(false),
      at_end_of_loop(true),
      read_from_loops_per_location(read_from_loops_per_location_) { }
  };
  ReadFromIfFoundData data(read_from_loops_per_location);
  if (m_first_iteration)
  {
    ASSERT(m_queued_actions.empty());
    // Look for the last write (if any) on the same thread.
    FollowVisitedOpsemHeads follow_visited_opsem_heads(m_read_action, visited_generation, data.new_path_condition);
    follow_visited_opsem_heads.process_queued(
        [this, &data](Action* action, boolean::Product const& path_condition)  // if_found
        {
          Dout(dc::notice, "ReadFromLoop::find_next_write_action: path_condition = " << path_condition << "; new_path_condition = " << data.new_path_condition);
          if (action->is_write())
          {
            Dout(dc::notice|continued_cf, "Found write " << *action << " if " << path_condition);
            store_write(action, boolean::Expression{path_condition}, data.found_write, true);
            Dout(dc::finish, ".");
            data.at_end_of_loop = false;
          }
          return true;  // Stop following edges once we found a sequenced before action for the same memory locaton.
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
