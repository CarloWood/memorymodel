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
  id_type m_batch_id;                   // The id of the first thread of the current batch of threads ({{{ ... }}}).
  ThreadPtr m_parent_thread;            // Only valid when m_id > 0.
  child_threads_type m_child_threads;   // Child threads of this thread.
  bool m_is_joined;                     // Set when this thread is joined.
  bool m_own_heads_added_back;          // Temporary variable used in join_all_threads.
  bool m_erase;                         // Set on the child thread by a call to joined(). Call do_erase in the parent thread afterwards.
  bool m_need_erase;                    // Set on the parent thread when any of the child threads has m_erase set. Call do_erase.

  int m_full_expression_detector_depth;                 // The current number of nested FullExpressionDetector objects.
  std::stack<BranchInfo> m_branch_info_stack;           // Stack to store BranchInfo for nested branches.
  EvaluationNodePtrConditionPairs m_unconnected_heads;  // List of currently unconnected heads with conditions.

 protected:
  Thread();
  Thread(id_type id, ThreadPtr const& parent_thread);
  ~Thread();

 public:
  // Create the main thread.
  static ThreadPtr create_main_thread() { return new Thread; }

  // Create a new child thread.
  ThreadPtr create_new_thread(id_type& next_id)
  {
    ThreadPtr child_thread{new Thread(next_id++, this)};
    m_child_threads.emplace_back(child_thread);
    return child_thread;
  }

  // Accessors.
  bool is_main_thread() const { return m_id == 0; }
  id_type id() const { return m_id; }                                                           // thread_id
  ThreadPtr const& parent_thread() const { ASSERT(m_id > 0); return m_parent_thread; }
  bool is_joined() const { return m_is_joined; }
  bool in_true_branch() const { return !m_branch_info_stack.empty() && m_branch_info_stack.top().in_true_branch(); }
  bool in_false_branch() const { return !m_branch_info_stack.empty() && !m_branch_info_stack.top().in_true_branch(); }

  // Called at sequence-points.
  void detect_full_expression_start();
  void detect_full_expression_end(Evaluation& full_expression);

  void start_threads(id_type next_id);
  void join_all_threads(id_type next_id);
  void joined();
  void clean_up_joined_child_threads();
  void scope_end();

  // Selection statement events.
  // We're about to execute the branch that is followed when this condition is true.
  void begin_branch_true(EvaluationNodePtrConditionPairs& unconnected_heads, std::unique_ptr<Evaluation>&& condition);
  // We're about to execute the branch that is followed when this condition is false.
  void begin_branch_false(EvaluationNodePtrConditionPairs& unconnected_heads);
  // Called immediately after the selection statement.
  void end_branch(EvaluationNodePtrConditionPairs& unconnected_heads);

  friend bool operator==(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id == thr2->m_id; }
  friend bool operator!=(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id != thr2->m_id; }
  friend bool operator<(ThreadPtr const& thr1, ThreadPtr const& thr2) { return thr1->m_id < thr2->m_id; }

  friend std::ostream& operator<<(std::ostream& os, Thread const& thread);
  friend std::ostream& operator<<(std::ostream& os, ThreadPtr const& thread) { return os << *thread; }

 private:
  // Actually erase child threads marked for erasure from m_child_threads.
  void do_erase();
};
