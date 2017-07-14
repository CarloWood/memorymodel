#include "sys.h"
#include "Graph.h"
#include "Context.h"
#include "debug.h"
#include "utils/macros.h"
#include <stdexcept>

std::ostream& operator<<(std::ostream& os, Thread const& thread)
{
  os << "{Thread: m_id = " << thread.m_id << "}";
  return os;
}

char const* access_str(Access access)
{
  switch (access)
  {
    AI_CASE_RETURN(ReadAccess);
    AI_CASE_RETURN(WriteAccess);
    AI_CASE_RETURN(MutexDeclaration);
    AI_CASE_RETURN(MutexLock1);
    AI_CASE_RETURN(MutexLock2);
    AI_CASE_RETURN(MutexUnlock1);
    AI_CASE_RETURN(MutexUnlock2);
  }
  throw std::runtime_error("Unknown Access type");
}

std::ostream& operator<<(std::ostream& os, Access access)
{
  return os << access_str(access);
}

std::ostream& operator<<(std::ostream& os, Node const& node)
{
  os << node.name() << ':' << node.type() << ' ' << node.tag() << '=';
  if (node.is_write())
    os << *node.m_evaluation;
  return os;
}

void Graph::print_nodes() const
{
  for (auto&& node : m_nodes)
  {
    Dout(dc::notice, '"' << node << '"');
  }
}

void Graph::scope_start(bool is_thread)
{
  DoutEntering(dc::notice, "Graph::scope_start('is_thread' = " << is_thread << ")");
  m_threads.push(is_thread);
  if (is_thread)
  {
    m_beginning_of_thread = true;
    m_current_thread = Thread::create_new_thread(m_next_thread_id, m_current_thread);
    DebugMarkDown;
    Dout(dc::notice, "Created " << m_current_thread << '.');
  }
}

void Graph::scope_end()
{
  bool is_thread = m_threads.top();
  DoutEntering(dc::notice, "Graph::scope_end() [is_thread = " << is_thread << "].");
  m_threads.pop();
  if (is_thread)
  {
    DebugMarkUp;
    Dout(dc::notice, "Joined thread " << m_current_thread << '.');
    m_current_thread = m_current_thread->parent_thread();
  }
}
