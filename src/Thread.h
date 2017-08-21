#pragma once

#include "utils/AIRefCount.h"
#include "debug.h"
#include <vector>
#include <memory>
#include <utility>
#include <iosfwd>
#include <functional>

class Evaluation;
class Thread;
using ThreadPtr = boost::intrusive_ptr<Thread>;

class Thread : public AIRefCount
{
 public:
  using id_type = int;
  using child_threads_type = std::vector<ThreadPtr>;

 private:
  id_type m_id;                         // Unique identifier of a thread.
  ThreadPtr m_parent_thread;            // Only valid when m_id > 0.
  child_threads_type m_child_threads;   // Child threads of this thread.
  std::unique_ptr<Evaluation> m_final_full_expression;  // The last full-expression of this thread.
  bool m_is_joined;                     // Set when this thread leaves its scope.

 protected:
  Thread();
  Thread(id_type id, ThreadPtr const& parent_thread);
  ~Thread() { ASSERT(m_id == 0 || m_is_joined); }

 public:
  static ThreadPtr create_main_thread() { return new Thread; }
  ThreadPtr create_new_thread(id_type& next_id)
  {
    ThreadPtr child_thread{new Thread(next_id++, this)};
    m_child_threads.emplace_back(child_thread);
    return child_thread;
  }
  bool is_main_thread() const { return m_id == 0; }
  id_type id() const { return m_id; }
  ThreadPtr const& parent_thread() const { ASSERT(m_id > 0); return m_parent_thread; }
  bool is_joined() const { return m_is_joined; }
  bool has_final_full_expression() const { return m_final_full_expression.get(); }
  Evaluation const& final_full_expression() const { return *m_final_full_expression; }

  void store_final_full_expression(std::unique_ptr<Evaluation>&& final_full_expression) { m_final_full_expression = std::move(final_full_expression); }
  void join_all_threads();
  void for_all_joined_child_threads(std::function<void(ThreadPtr const&)> const& final_full_expression);
  void joined() { ASSERT(!m_is_joined); m_is_joined = true; }
  void clean_up_joined_child_threads();

  friend bool operator==(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id == thr2->m_id; }
  friend bool operator!=(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id != thr2->m_id; }
  friend bool operator<(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id < thr2->m_id; }

  friend std::ostream& operator<<(std::ostream& os, Thread const& thread);
  friend std::ostream& operator<<(std::ostream& os, ThreadPtr const& thread) { return os << *thread; }
};
