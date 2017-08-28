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
  bool m_at_beginning_of_child_thread;  // Set to true upon creation when this is a child thread. Reset at the first full-expression.
  bool m_finished;                      // Set at the end of the scope of this thread when this is a child thread.
  bool m_is_joined;                     // Set when this thread is joined.
  int m_pending_heads;                  // Number of not-yet-connected heads (added as NodePtrConditionPair, created by add_unconnected_head_nodes).
                                        // This combines the count of unconnected heads of true- and false-branches when needed.
  bool m_erase;                         // Set on the child thread by a call to joined_and_connected(child). Call do_erase in the parent thread afterwards.
  bool m_need_erase;                    // Set on the parent thread when any of the child threads has m_erase set. Call do_erase.
  bool const m_parent_in_true_branch;   // Set when we were created in a true-branch of the parent thread.

  bool m_joined_child_threads;          // Set when one or more child threads were just joined. Reset the next full-expression in this thread.
  bool m_true_branch_joined_child_threads;// See also the note in add_unconnected_head_nodes.
  bool m_saw_empty_child_thread;        // Set when the scope of an empty child threads was just closed. Reset the next full-expression in this thread.
  bool m_true_branch_saw_empty_child_thread;

  int m_full_expression_detector_depth;                                 // The current number of nested FullExpressionDetector objects.
  BranchInfo::full_expression_evaluations_type& m_full_expression_evaluations;      // Convenience reference to Context::m_full_expression_evaluations.
  std::unique_ptr<Evaluation> m_previous_full_expression;               // The previous full expression.
  std::stack<BranchInfo> m_branch_info_stack;                           // Stack to store BranchInfo for nested branches.
  bool m_unhandled_condition;                                           // Set when an unconnected condition node has been returned by add_unconnected_head_nodes.
  std::stack<BranchInfo> m_finalize_branch_stack;                       // Stack to store BranchInfo for branches that need to be finalized.
  size_t m_protected_finalize_branch_stack_size;                        // The number of BranchInfo elements at the beginning of
                                                                        //  m_finalize_branch_stack that should not be finalized yet.

 protected:
  Thread(full_expression_evaluations_type& full_expression_evaluations);
  Thread(full_expression_evaluations_type& full_expression_evaluations, id_type id, ThreadPtr const& parent_thread);
  ~Thread() { Dout(dc::threads, "Destructing Thread with ID " << m_id); ASSERT(m_id == 0 || (m_is_joined && m_pending_heads == 0)); }

 public:
  // Create the main thread.
  static ThreadPtr create_main_thread(full_expression_evaluations_type& full_expression_evaluations) { return new Thread(full_expression_evaluations); }

  // Create a new child thread.
  ThreadPtr create_new_thread(full_expression_evaluations_type& full_expression_evaluations, id_type& next_id)
  {
    ThreadPtr child_thread{new Thread(full_expression_evaluations, next_id++, this)};
    m_child_threads.emplace_back(child_thread);
    return child_thread;
  }

  // Accessors.
  bool is_main_thread() const { return m_id == 0; }
  id_type id() const { return m_id; }
  ThreadPtr const& parent_thread() const { ASSERT(m_id > 0); return m_parent_thread; }
  bool is_joined() const { return m_is_joined; }
  bool in_true_branch() const { return !m_branch_info_stack.empty() && m_branch_info_stack.top().in_true_branch(); }
  bool in_false_branch() const { return !m_branch_info_stack.empty() && !m_branch_info_stack.top().in_true_branch(); }
  bool created_in_true_branch() const { return m_parent_in_true_branch; }

  // Called at sequence-points.
  void detect_full_expression_start();
  void detect_full_expression_end(Evaluation& full_expression, Context& context);

  void start_threads(id_type next_id);
  void join_all_threads(id_type next_id);
  void for_all_joined_child_threads(std::function<void(ThreadPtr const&)> const& final_full_expression);
  void joined();
  void clean_up_joined_child_threads();
  void joined_and_connected(ThreadPtr const& child_thread_ptr);
  void connected();
  void closed_child_thread_with_full_expression(bool had_full_expression);
  void scope_end();
  void saw_empty_child(ThreadPtr const& child);

  // Selection statement events.
  // The previous full-expression is a condition and we're about to execute the branch that is followed when this condition is true.
  void begin_branch_true(Context& context);
  // We're about to execute the branch that is followed when this condition is false (previous full-expression is the last full-expression
  // of the true-branch here (or the condition if the true-branch is empty).
  void begin_branch_false();
  // Called immediately after the selection statement.
  void end_branch();

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
  void add_unconnected_head_nodes(EvaluationNodePtrConditionPairs& before_node_ptr_condition_pairs, bool child_threads);
  // Called when the current branch condition is connected with sb or asw edges.
  void condition_handled();
  // Actually erase child threads marked for erasure from m_child_threads.
  void do_erase();
};
