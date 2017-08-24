#pragma once

#include "utils/AIRefCount.h"
#include "BranchInfo.h"
#include "EvaluationNodePtrs.h"
#include "debug.h"
#include <vector>
#include <memory>
#include <utility>
#include <iosfwd>
#include <functional>
#include <stack>

struct Context;
class Evaluation;
class Thread;
using ThreadPtr = boost::intrusive_ptr<Thread>;

class Thread : public AIRefCount
{
 public:
  using id_type = int;
  using child_threads_type = std::vector<ThreadPtr>;
  using full_expression_evaluations_type = BranchInfo::full_expression_evaluations_type;

 private:
  id_type m_id;                         // Unique identifier of a thread.
  id_type m_batch_id;                   // The id of the first thread of the current batch of threads ({{{ ... }}}).
  ThreadPtr m_parent_thread;            // Only valid when m_id > 0.
  child_threads_type m_child_threads;   // Child threads of this thread.
  bool m_is_joined;                     // Set when this thread leaves its scope.
  int m_pending_heads;                  // Number of not-yet-connected heads (added as CurrentHeadOfThread).

  int m_full_expression_detector_depth;                                 // The current number of nested FullExpressionDetector objects.
  BranchInfo::full_expression_evaluations_type& m_full_expression_evaluations;      // Convenience reference to Context::m_full_expression_evaluations.
  std::unique_ptr<Evaluation> m_previous_full_expression;               // The previous full expression, if not a condition
                                                                        //  (aka m_condition is 1),
                                                                        //  otherwise use m_last_condition.
  std::stack<BranchInfo> m_branch_info_stack;                           // Stack to store BranchInfo for nested branches.
  std::stack<BranchInfo> m_finalize_branch_stack;                       // Stack to store BranchInfo for branches that need to be finalized.
  size_t m_protected_finalize_branch_stack_size;                        // The number of BranchInfo elements at the beginning of
                                                                        //  m_finalize_branch_stack that should not be finalized yet.

 protected:
  Thread(full_expression_evaluations_type& full_expression_evaluations);
  Thread(full_expression_evaluations_type& full_expression_evaluations, id_type id, ThreadPtr const& parent_thread);
  ~Thread() { Dout(dc::threads, "Destructing Thread with ID " << m_id); ASSERT(m_id == 0 || (m_is_joined && m_pending_heads == 0)); }

 public:
  static ThreadPtr create_main_thread(full_expression_evaluations_type& full_expression_evaluations) { return new Thread(full_expression_evaluations); }
  ThreadPtr create_new_thread(full_expression_evaluations_type& full_expression_evaluations, id_type& next_id)
  {
    ThreadPtr child_thread{new Thread(full_expression_evaluations, next_id++, this)};
    m_child_threads.emplace_back(child_thread);
    return child_thread;
  }
  bool is_main_thread() const { return m_id == 0; }
  id_type id() const { return m_id; }
  ThreadPtr const& parent_thread() const { ASSERT(m_id > 0); return m_parent_thread; }
  bool is_joined() const { return m_is_joined; }
  bool has_previous_full_expression() const { return m_previous_full_expression.get(); }
  Evaluation const& previous_full_expression() const { return *m_previous_full_expression; }

  // Called at sequence-points.
  void detect_full_expression_start();
  void detect_full_expression_end(Evaluation& full_expression, Context& context);

  void start_threads(id_type next_id);
  void join_all_threads(id_type next_id);
  void for_all_joined_child_threads(std::function<void(ThreadPtr const&)> const& final_full_expression);
  void joined() { Dout(dc::threads, "Calling joined() for thread with ID " << m_id); ASSERT(!m_is_joined); m_is_joined = true; }
  void clean_up_joined_child_threads();
  void joined_and_connected(ThreadPtr const& child_thread_ptr);
  void connected();

  // The previous full-expression is a condition and we're about to execute
  // the branch that is followed when this condition is true.
  void begin_branch_true(Context& context);

  void begin_branch_false()
  {
    DoutEntering(dc::branch, "Thread::begin_branch_with_condition()");
    Dout(dc::branch|dc::fullexpr, "Moving m_previous_full_expression to begin_branch_false(): (see MOVING)");
    m_branch_info_stack.top().begin_branch_false(std::move(m_previous_full_expression));
  }

  void end_branch()
  {
    DoutEntering(dc::branch, "Thread::end_branch_with_condition()");
    Dout(dc::branch|dc::fullexpr, "Moving m_previous_full_expression to end_branch(): (see MOVING)");
    m_branch_info_stack.top().end_branch(std::move(m_previous_full_expression));

    Dout(dc::branch, "Moving " << m_branch_info_stack.top() << " from m_branch_info_stack to m_finalize_branch_stack.");
    m_finalize_branch_stack.push(m_branch_info_stack.top());
    m_branch_info_stack.pop();
  }

  size_t protect_finalize_branch_stack()
  {
    DoutEntering(dc::branch, "Thread::protect_finalize_branch_stack()");
    size_t old_protected_finalize_branch_stack_size = m_protected_finalize_branch_stack_size;
    m_protected_finalize_branch_stack_size = m_finalize_branch_stack.size();
    Dout(dc::branch, "Set m_protected_finalize_branch_stack_size to " << m_protected_finalize_branch_stack_size <<
        "; old value was " << old_protected_finalize_branch_stack_size << ".");
    return old_protected_finalize_branch_stack_size;
  }

  void unprotect_finalize_branch_stack(int old_protected_finalize_branch_stack_size)
  {
    DoutEntering(dc::branch, "Thread::unprotect_finalize_branch_stack()");
    Dout(dc::branch, "Restoring m_protected_finalize_branch_stack_size to previous value (" << old_protected_finalize_branch_stack_size << ").");
    m_protected_finalize_branch_stack_size = old_protected_finalize_branch_stack_size;
  }

  friend bool operator==(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id == thr2->m_id; }
  friend bool operator!=(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id != thr2->m_id; }
  friend bool operator<(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id < thr2->m_id; }

  friend std::ostream& operator<<(std::ostream& os, Thread const& thread);
  friend std::ostream& operator<<(std::ostream& os, ThreadPtr const& thread) { return os << *thread; }

 private:
  // Add node/condition pairs to current_heads_of_thread for all head nodes of the current thread or all child_threads.
  void add_previous_nodes(EvaluationCurrentHeadsOfThread& current_heads_of_thread, bool child_threads);

#ifdef CWDEBUG
 private:
  bool m_first_full_expression;
#endif
};
