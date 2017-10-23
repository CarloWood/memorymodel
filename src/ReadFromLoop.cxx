#include "sys.h"
#include "ReadFromLoop.h"
#include "FollowVisitedOpsemHeads.h"
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

bool ReadFromLoop::find_next_write_action(ReadFromLoopsPerLocation& read_from_loops_per_location, int visited_generation)
{
#ifdef CWDEBUG
  DoutEntering(dc::notice, "find_next_write_action() on ReadFromLoop for read action " << *m_read_action);
  debug::Mark marker{m_read_action->name().c_str()};
#endif
  if (m_topo_next == m_topo_end)
    Dout(dc::notice, "m_topo_next == m_topo_end");
  else
    Dout(dc::notice, "m_topo_next points to " << **m_topo_next);
  // g++ starts allocating memory for lambda's capturing more than 16 bytes (clang++ 24 bytes), so put all data that we need in a struct.
  struct ReadFromIfFoundData
  {
    boolean::Expression found_write;    // Boolean expression under which we found a write.
    bool have_sequenced_before_writes;  // Set to true when we found one or more write on the same or joined threads.
    bool at_end_of_loop;                // Set to true when all writes have been found.
    ReadFromLoopsPerLocation& read_from_loops_per_location;     // Reference to the list of all ReadFromLoops.

    ReadFromIfFoundData(ReadFromLoopsPerLocation& read_from_loops_per_location_) :
      found_write(false),
      have_sequenced_before_writes(false),
      at_end_of_loop(false),
      read_from_loops_per_location(read_from_loops_per_location_) { }
  };
  ReadFromIfFoundData data(read_from_loops_per_location);
  if (m_first_iteration)
  {
    Dout(dc::notice, "m_first_iteration is true.");
    ASSERT(m_queued_actions.empty());
    // Look for the last write (if any) on the same thread.
    FollowVisitedOpsemHeads follow_visited_opsem_heads(m_read_action, visited_generation);
    follow_visited_opsem_heads.process_queued(
        [this, &data](Action* action, boolean::Expression&& path_condition)  // if_found
        {
          Dout(dc::notice, "path_condition = " << path_condition);
          if (action->is_write())
          {
            Dout(dc::notice|continued_cf, "Found write " << *action << " if " << path_condition);
            store_write(action, std::move(path_condition), data.found_write, true);
            Dout(dc::finish, ".");
          }
          else
          {
            // Can it happen that this is not the case?
            ASSERT(action->is_read());
            Dout(dc::notice, "Found read " << *action << " if " << path_condition);
            ReadFromLoop const& read_from_loop{data.read_from_loops_per_location.get_read_from_loop_of(action)};
            // This read is already finished, so if it has no writes that it reads from then it would be
            // reading uninitialized data. If this turns out to be normal then data.have_sequenced_before_writes
            // should be set accordingly.
            write_actions_type::const_iterator read_from = read_from_loop.m_write_actions.begin();
            ASSERT(read_from != read_from_loop.m_write_actions.end());
            // If any write (ie, the first one) is not sequenced before the read (action) then none of them will be,
            // because we never return a mix of those. If they are sequenced before the read then we can't stop
            // at this read because the algorihm that finds writes that are sequenced before the corresponding
            // read needs us to mark every edge in between to be marked with a 'visited' boolean expression.
            if (!action->is_sequenced_before(*read_from->first))
              return false;
            do
            {
              Dout(dc::notice|continued_cf, "  reading from " << read_from->first->name() << " when " << read_from->second);
              boolean::Expression condition{read_from->second.times(path_condition)};
              store_write(read_from->first, std::move(condition), data.found_write, true);
              Dout(dc::finish, ".");
            }
            while (++read_from != read_from_loop.m_write_actions.end());
          }
          data.have_sequenced_before_writes = true;
          return true;  // Stop following edges once we found a sequenced before action for the same memory locaton.
        }
    );
    m_first_iteration = false;
  }
  else
  {
    Dout(dc::notice, "m_first_iteration is false.");
    data.have_sequenced_before_writes = !m_queued_actions.empty();
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
  if (!data.have_sequenced_before_writes)
  {
    Dout(dc::notice, "have_sequenced_before_writes is false; starting next phase (unsequenced writes).");
    data.at_end_of_loop = true;
    if (m_topo_next == m_topo_end)
      Dout(dc::notice, "m_topo_next == m_topo_end");
    while (m_topo_next != m_topo_end)
    {
      Dout(dc::notice, "m_topo_next points to " << **m_topo_next);
      Dout(dc::warning, "******************************************************* WE GET HERE *************************");
      if ((*m_topo_next)->thread() != m_read_action->thread() &&
          (*m_topo_next)->location() == m_read_action->location() &&
          (*m_topo_next)->is_write() &&
         !(*m_topo_next)->is_sequenced_before(*m_read_action) &&
         !m_read_action->is_sequenced_before(**m_topo_next))
      {
        boolean::Expression condition{true};
        Dout(dc::notice|continued_cf, "Found unsequenced write " << **m_topo_next);
        store_write(*m_topo_next, std::move(condition), data.found_write, true);
        Dout(dc::finish, ".");
        data.at_end_of_loop = false;
      }
      ++m_topo_next;
    }
  }
  if (m_topo_next == m_topo_end)
    Dout(dc::notice, "m_topo_next == m_topo_end");
  else
    Dout(dc::notice, "m_topo_next points to " << **m_topo_next);
  return !data.at_end_of_loop;
}
