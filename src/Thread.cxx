#include "sys.h"
#include "Thread.h"
#include "Evaluation.h"
#include "SBNodePresence.h"
#include "Context.h"
#include "NodePtrConditionPair.h"
#include <algorithm>

std::ostream& operator<<(std::ostream& os, Thread const& thread)
{
  os << "{Thread: m_id:" << thread.m_id <<
    ", m_is_joined:" << thread.m_is_joined <<
//    ", m_pending_heads:" << thread.m_pending_heads <<
//    ", m_joined_child_threads:" << thread.m_joined_child_threads <<
//    ", m_saw_empty_child_thread:" << thread.m_saw_empty_child_thread <<
    "}";
  return os;
}

void Thread::start_threads(id_type next_id)
{
  DoutEntering(dc::threads, "Thread::start_threads(" << next_id << ") with m_id = " << m_id);
  m_batch_id = next_id;
}

void Thread::join_all_threads(id_type next_id)
{
  DoutEntering(dc::threads, "Thread::join_all_threads(" << next_id << ") with m_id = " << m_id);
  // Assume that all our unconnected heads have been connected to child threads.
  m_unconnected_heads.clear();
  // If that is not the case then that child thread will try to pass them back
  // to use; use this boolean to only allow one child thread to do that.
  m_own_heads_added_back = false;
  // Join all threads of current batch.
  for (auto&& child_thread : m_child_threads)
    if (child_thread->id() >= m_batch_id)
      child_thread->joined();
  // Actually erase the child threads, if any.
  if (m_need_erase)
    do_erase();
  // Make sure we won't join them again.
  m_batch_id = next_id;
}

void Thread::joined()
{
  Dout(dc::threads, "Calling joined() for thread with ID " << m_id);
  ASSERT(!m_is_joined);
  m_is_joined = true;
  // Pass remaining unconnected heads to the parent thread.
  m_parent_thread->m_unconnected_heads += m_unconnected_heads;
  m_erase = true;
  m_parent_thread->m_need_erase = true;
}

void Thread::do_erase()
{
  DoutEntering(dc::threads, "Thread::do_erase()");
  m_child_threads.erase(
      std::remove_if(
        m_child_threads.begin(),
        m_child_threads.end(),
        [](ThreadPtr const& thread)
        {
#ifdef CWDEBUG
          if (thread->m_erase)
            Dout(dc::threads, "Erasing thread " << thread->m_id);
#endif
          return thread->m_erase;
        }
      ),
      m_child_threads.end());
  m_need_erase = false;
}

Thread::Thread() :
  m_id(0),
  m_is_joined(false),
  m_erase(false),
  m_need_erase(false),
  m_full_expression_detector_depth(0)
{
}

Thread::Thread(id_type id, ThreadPtr const& parent_thread) :
  m_id(id),
  m_parent_thread(parent_thread),
  m_is_joined(false),
  m_erase(false),
  m_need_erase(false),
  m_full_expression_detector_depth(0),
  m_unconnected_heads(parent_thread->m_unconnected_heads)
{
  ASSERT(m_id > 0);
}

Thread::~Thread()
{
  Dout(dc::threads, "Destructing Thread with ID " << m_id);
  ASSERT(m_id == 0 || m_is_joined);
  ASSERT(m_child_threads.empty());
}

void Thread::scope_end()
{
  DoutEntering(dc::threads, "Thread::scope_end() [this = " << *this << "].");
}

void Thread::detect_full_expression_start()
{
  ++m_full_expression_detector_depth;
}

void Thread::detect_full_expression_end(Evaluation& full_expression, Context& context)
{
  // An expression that is not part of another full-expression is a full-expression.
  if (--m_full_expression_detector_depth == 0)
  {
    // full_expression is the evaluation of a full-expression.
    Dout(dc::fullexpr, "Found full-expression with evaluation: " << full_expression);

    // An Evaluation is allocated iff when the expression is pointed to by a std::unique_ptr,
    // which are only used as member variables of another Evaluation; hence a full-expression
    // which can never be a part of another (full-)expression, will never be allocated.
    ASSERT(!full_expression.is_allocated());

    EvaluationNodePtrs full_expression_nodes =
        full_expression.get_nodes(NodeRequestedType::tails COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));
    Dout(dc::fullexpr, "Found nodes in full-expression: " << full_expression_nodes);

    if (!full_expression_nodes.empty())
    {
      context.add_sb_or_asw_edges(m_unconnected_heads, full_expression_nodes);

      m_unconnected_heads = full_expression.get_nodes(NodeRequestedType::heads COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));
      Dout(dc::fullexpr, "New m_unconnected_heads is: " << m_unconnected_heads << ".");
    }
  }
}

void Thread::begin_branch_true(EvaluationNodePtrConditionPairs& unconnected_heads, std::unique_ptr<Evaluation>&& condition, Context& context)
{
  DoutEntering(dc::branch, "Thread::begin_branch_true(" << *condition << ")");
  // Here we are directly after an 'if ()' statement.
  // `condition` is the conditional expression of a selection statement (and assumed true here).
  ConditionalBranch conditional_branch{context.add_condition(std::move(condition))};
  // Create a new BranchInfo for this selection statement.
  m_branch_info_stack.emplace(conditional_branch);
  Dout(dc::branch, "Added " << m_branch_info_stack.top() << " to m_branch_info_stack.");

  // Prepare the unconnected heads for the false-branch.
  unconnected_heads = m_unconnected_heads;
  unconnected_heads *= m_branch_info_stack.top().get_negated_current_condition();

  // Turn unconnected heads into those of the true-branch.
  m_unconnected_heads *= m_branch_info_stack.top().get_current_condition();
}

void Thread::begin_branch_false(EvaluationNodePtrConditionPairs& unconnected_heads)
{
  DoutEntering(dc::branch, "Thread::begin_branch_false()");
  m_branch_info_stack.top().begin_branch_false();

  // Swap true- and false- branch unconnected heads.
  unconnected_heads.swap(m_unconnected_heads);
}

void Thread::end_branch(EvaluationNodePtrConditionPairs& unconnected_heads)
{
  DoutEntering(dc::branch, "Thread::end_branch()");
  m_branch_info_stack.top().end_branch();
  m_branch_info_stack.pop();

  // Combine true- and false- branch unconnected heads.
  m_unconnected_heads += unconnected_heads;
}

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct fullexpr("FULLEXPR");
channel_ct threads("THREADS");
NAMESPACE_DEBUG_CHANNELS_END
#endif
