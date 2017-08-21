#include "sys.h"
#include "Thread.h"
#include "Evaluation.h"

void Thread::join_all_threads()
{
  for (auto&& child_thread : m_child_threads)
    child_thread->joined();
}

void Thread::for_all_joined_child_threads(std::function<void(ThreadPtr const&)> const& action)
{
  for (auto&& child_thread : m_child_threads)
  {
    child_thread->for_all_joined_child_threads(action);
    if (child_thread->is_joined())
      action(child_thread);
  }
}

void Thread::clean_up_joined_child_threads()
{
  if (m_child_threads.empty())
    return;
  child_threads_type::iterator child = m_child_threads.end();
  while (child != m_child_threads.begin())
    if ((*--child)->is_joined())
      child = m_child_threads.erase(child);
}

Thread::Thread() : m_id(0), m_is_joined(false)
{
}

Thread::Thread(id_type id, ThreadPtr const& parent_thread) : m_id(id), m_parent_thread(parent_thread), m_is_joined(false)
{
  ASSERT(m_id > 0);
}
