#include "sys.h"
#include "Thread.h"
#include "Evaluation.h"
#include "SBNodePresence.h"
#include "Context.h"

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

Thread::Thread(full_expression_evaluations_type& full_expression_evaluations) :
  m_id(0),
  m_is_joined(false),
  m_full_expression_detector_depth(0),
  m_full_expression_evaluations(full_expression_evaluations),
  m_protected_finalize_branch_stack_size(0),
  m_first_full_expression(true)
{
}

Thread::Thread(full_expression_evaluations_type& full_expression_evaluations, id_type id, ThreadPtr const& parent_thread) :
  m_id(id),
  m_parent_thread(parent_thread),
  m_is_joined(false),
  m_full_expression_detector_depth(0),
  m_full_expression_evaluations(full_expression_evaluations),
  m_protected_finalize_branch_stack_size(0),
  m_first_full_expression(true)
{
  ASSERT(m_id > 0);
}

void Thread::detect_full_expression_start()
{
  ++m_full_expression_detector_depth;
}

void Thread::detect_full_expression_end(Evaluation& full_expression)
{
  // An expression that is not part of another full-expression is a full-expression.
  if (--m_full_expression_detector_depth == 0)
  {
    // full_expression is the evaluation of a full-expression.
    Dout(dc::notice, "Found full-expression with evaluation: " << full_expression);

    // An Evaluation is allocated iff when the expression is pointed to by a std::unique_ptr,
    // which are only used as member functions of another Evaluation; hence a full-expression
    // which can never be a part of another (full-)expression, will never be allocated.
    ASSERT(!full_expression.is_allocated());

    // Count number of nodes in full_expression.
    int number_of_nodes = 0;
    full_expression.for_each_node(NodeRequestedType::tails, [&number_of_nodes](NodePtr const&) { ++number_of_nodes; } COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));
    Dout(dc::notice, "number_of_nodes = " << number_of_nodes);

    if (number_of_nodes > 0)
    {

#if 0
      // Add asw edges.
      if (m_beginning_of_thread)
      {
        m_beginning_of_thread = false;
        if (!m_last_full_expressions.empty())     // Can happen when we have an empty thread.
          add_edges(edge_asw, *m_last_full_expressions.top(), full_expression COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::asw_edge));
      }
      if (m_end_of_thread)
      {
        m_end_of_thread = false;
        m_current_thread->for_all_joined_child_threads(
            [this, &full_expression](ThreadPtr const& child_thread)
            {
              if (child_thread->has_final_full_expression())
                add_edges(edge_asw, child_thread->final_full_expression(), full_expression COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::asw_edge));
            }
        );
        m_current_thread->clean_up_joined_child_threads();
      }
#endif

      // This will be non-zero if we're inside some selection statement.
      Dout(dc::branch, "Size of m_branch_info_stack is " << m_branch_info_stack.size() << ".");
      // Let branch_info point to the inner most selection statement that we're current in, if any.
      BranchInfo* const branch_info = m_branch_info_stack.empty() ? nullptr : &m_branch_info_stack.top();
      // Detect if this full-expression is the first full-expression in the current branch.
      bool first_full_expression_of_current_branch = branch_info && !branch_info->conditional_edge_of_current_branch_added();

#ifdef CWDEBUG
      if (first_full_expression_of_current_branch)
        Dout(dc::branch, "The previous full-expression is the CONDITION " << *m_full_expression_evaluations[branch_info->m_condition_index]);
      else
        Dout(dc::branch, "The previous full-expression is NOT the CONDITION.");
#endif

      // If this is the first full-expression of a current branch then m_previous_full_expression
      // is a condition and was moved to m_full_expression_evaluations.
      std::unique_ptr<Evaluation> const& previous_full_expression{
          first_full_expression_of_current_branch ?
              m_full_expression_evaluations[branch_info->m_condition_index] :
              m_previous_full_expression};

      bool const previous_full_expression_is_valid{previous_full_expression};
      bool just_after_selection_statement = m_finalize_branch_stack.size() > m_protected_finalize_branch_stack_size;

      Dout(dc::branch, "first_full_expression_of_current_branch = " << first_full_expression_of_current_branch);
      Dout(dc::branch, "previous_full_expression_is_valid = " << previous_full_expression_is_valid);

#ifdef CWDEBUG
      debug::Mark* marker;
      if (previous_full_expression_is_valid || just_after_selection_statement)
      {
        Dout(dc::sb_edge|continued_cf, "Generate sequenced-before edges between ");
        if (previous_full_expression_is_valid)
        {
          if (just_after_selection_statement)
            Dout(dc::continued, "selection statement with last full-expression " << *previous_full_expression);
          else
            Dout(dc::continued, *previous_full_expression);
        }
        else
          Dout(dc::continued, " selection statement with no last full-expression in its last branch.");
        Dout(dc::finish, " and " << full_expression << ".");
        Debug(marker = new debug::Mark("\e[43;33;1m↳\e[0m"));    // DebugMarkDownRight
        m_first_full_expression = false;
      }
      else if (!m_first_full_expression)
      {
        if (!just_after_selection_statement)
        {
          // m_previous_full_expression should only be invalid when we encounter the first full-expression.
          // After that it should always be equal to the previous full-expression.
          if (m_full_expression_evaluations.empty())
            DoutFatal(dc::core, "Unexpected invalid m_previous_full_expression! Current full-expression is " << full_expression);
          else
            DoutFatal(dc::core, "Unexpected invalid m_previous_full_expression! Last full-expression is " << *m_full_expression_evaluations.back());
        }
      }
#endif

      if (previous_full_expression_is_valid || just_after_selection_statement)
      {
        std::vector<Evaluation::node_pairs_type> skip_edge_node_pairs;
        std::vector<Condition> skip_conditions;

        Dout(dc::branch, "Size of m_finalize_branch_stack = " << m_finalize_branch_stack.size());
        // Only finalize selection statements that have finished; NOT stuff from the
        // true-branch while we're still processing the false-branch here.
        //
        //   if (condition1)
        //     //true-branch
        //   else
        //   {
        //     //false-branch
        //     if (condition2)
        //     {
        //       ...
        //     }
        //     // Finalize if (condition2) ..., not if (condition1)'s true-branch yet.
        //   }
        while (m_finalize_branch_stack.size() > m_protected_finalize_branch_stack_size)
        {
          BranchInfo& branch_info{m_finalize_branch_stack.top()};

          // Handle four cases:
          // 0: unconditional edge from condition, or condition == True edge from condition.
          // 1: condition == False edge from condition.
          // 2: from last full-expression of True branch.
          // 3: from last full-expression of False branch.
          for (int cs = 0; cs < 4; ++cs)
          {
            int index = -1;
            Condition condition;
            switch (cs)
            {
              case 0:
                if (!branch_info.m_edge_to_true_branch_added)
                {
                  if (!branch_info.m_edge_to_false_branch_added)
                  {
                    // Unconditional edge from condition.
                    index = branch_info.m_condition_index;
                    branch_info.m_edge_to_true_branch_added = branch_info.m_edge_to_false_branch_added = true;
                  }
                  else
                  {
                    // Conditional edge for when the condition is true.
                    index = branch_info.m_condition_index;
                    condition = branch_info.m_condition(true);
                    branch_info.m_edge_to_true_branch_added = true;
                  }
                }
                break;
              case 1:
                if (!branch_info.m_edge_to_false_branch_added)
                {
                  // Conditional edge for when the condition is false;
                  index = branch_info.m_condition_index;
                  condition = branch_info.m_condition(false);
                  branch_info.m_edge_to_false_branch_added = true;
                }
                break;
              case 2:
                // Unconditional edge from last full-expression of true branch.
                index = branch_info.m_last_full_expression_of_true_branch_index;
                break;
              case 3:
                // Unconditional edge from last full-expression of false branch.
                index = branch_info.m_last_full_expression_of_false_branch_index;
                break;
            }
            if (index == -1)
              continue;

#ifdef CWDEBUG
            Dout(dc::branch|dc::sb_edge|continued_cf, "Finalize: generate edges between between " << (cs <= 1 ? "conditional " : "") <<
                *m_full_expression_evaluations[index] << " and " << full_expression);
            if (condition.conditional())
              Dout(dc::continued, " with condition " << condition);
            Dout(dc::finish, ".");
            DebugMarkDownRight;
#endif

#if 0
            // Generate skip edge node pairs.
            skip_edge_node_pairs.emplace_back(
                generate_node_pairs(
                    m_full_expression_evaluations[index]->get_nodes(
                        NodeRequestedType::heads
                        COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge)),
                    full_expression
                    COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge)));
#endif
            // Also remember the condition to use for those.
            skip_conditions.push_back(condition);
          }

          // Pop branch info from stack.
          Dout(dc::branch, "Removing " << branch_info << " from m_finalize_branch_stack because it was handled.");
          m_finalize_branch_stack.pop();
        }

        if (first_full_expression_of_current_branch)
	{
          Condition condition = branch_info->get_current_condition();
          Dout(dc::branch|dc::sb_edge, "Generate edges between between CONDITIONAL " << *previous_full_expression << " and " << full_expression <<
              " using boolean expression " << condition << ".");
          DebugMarkDownRight;

          // Get all head nodes of the last full expression (which is the condition).
          EvaluationNodePtrs previous_full_expression_nodes =
              previous_full_expression->get_nodes(NodeRequestedType::heads COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));

#if 0
          add_edges(edge_sb,
              generate_node_pairs(previous_full_expression_nodes, full_expression COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge))
              COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge), condition);
#endif
          Dout(dc::branch, "Marking that these edges were handled.");
          branch_info->added_edge_from_condition();
	}
#if 0
        else if (previous_full_expression_is_valid)
          add_edges(edge_sb, *m_previous_full_expression, full_expression COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));

        // Add the skip edges.
        int i = 0;
        for (auto&& skip_edges : skip_edge_node_pairs)
          add_edges(edge_sb, skip_edges COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge), skip_conditions[i++]);
#endif
      }
      m_previous_full_expression = Evaluation::make_unique(std::move(full_expression));
      Dout(dc::notice, "SET m_previous_full_expression to full_expression (" << *m_previous_full_expression << ").");

#ifdef CWDEBUG
      if (previous_full_expression_is_valid || just_after_selection_statement)
        delete marker;
#endif
    }
  }
}

void Thread::begin_branch_true(Context& context)
{
  DoutEntering(dc::branch, "Thread::begin_branch_true()");
  // Here we are directly after an 'if ()' statement.
  // m_previous_full_expression must be the conditional
  // expression that was tested (and assumed true here).
  ASSERT(m_previous_full_expression);
  Dout(dc::branch, "Adding condition from m_previous_full_expression (" << *m_previous_full_expression << ")...");
  ConditionalBranch conditional_branch{context.add_condition(m_previous_full_expression)};    // The previous full-expression is a conditional.
  // Create a new BranchInfo for this selection statement.
  Dout(dc::branch, "Moving m_previous_full_expression to BranchInfo(): see MOVING...");
  m_branch_info_stack.emplace(conditional_branch, m_full_expression_evaluations, std::move(m_previous_full_expression));
  Dout(dc::branch, "Added " << m_branch_info_stack.top() << " to m_branch_info_stack, and returning " << conditional_branch << ".");
}
