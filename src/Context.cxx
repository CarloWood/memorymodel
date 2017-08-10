#include "sys.h"
#include "Context.h"
#include "Graph.h"
#include "debug_ostream_operators.h"

#ifdef CWDEBUG
// To make DoutTag work.
#define context (*this)
#endif

std::ostream& operator<<(std::ostream& os, Context::ConditionalBranch const& conditional_branch)
{
  os << '{' << *conditional_branch.m_conditional->first << ", " << conditional_branch.m_conditional->second << '}';
  return os;
}

void Context::scope_start(bool is_thread)
{
  DoutEntering(dc::notice, "Context::scope_start('is_thread' = " << is_thread << ")");
  m_threads.push(is_thread);
  if (is_thread)
  {
    m_beginning_of_thread = true;
    m_current_thread = Thread::create_new_thread(m_next_thread_id, m_current_thread);
    Dout(dc::notice, "MOVING m_previous_full_expression (" << *m_previous_full_expression << ") to m_last_full_expressions.");
    m_last_full_expressions.emplace(std::move(m_previous_full_expression));
    DebugMarkDown;
    Dout(dc::notice, "Created " << m_current_thread << '.');
  }
}

void Context::scope_end()
{
  bool is_thread = m_threads.top();
  DoutEntering(dc::notice, "Context::scope_end() [is_thread = " << is_thread << "].");
  m_threads.pop();
  if (is_thread)
  {
    DebugMarkUp;
    Dout(dc::notice, "Joined thread " << m_current_thread << '.');
    m_current_thread = m_current_thread->parent_thread();
    m_previous_full_expression = std::move(m_last_full_expressions.top());
    m_last_full_expressions.pop();
    Dout(dc::notice, "RESTORED m_previous_full_expression from m_last_full_expressions to " << *m_previous_full_expression << ".");
  }
}

Evaluation Context::uninitialized(ast::tag decl)
{
  DoutTag(dc::notice, "[uninitialized declaration of", decl);
  Evaluation result;    // Uninitialzed.
  return result;
}

void Context::read(ast::tag variable, Evaluation& evaluation)
{
  DoutTag(dc::notice, "[NA read from", variable);
  auto new_node = m_graph.new_node<NAReadNode>(m_current_thread, variable);
  // Should be added as side effect when variable is volatile.
  evaluation.add_value_computation(new_node);
}

void Context::write(ast::tag variable, Evaluation&& evaluation, bool side_effect_sb_value_computation)
{
  DoutTag(dc::notice, "[NA write to", variable);
  auto write_node_ptr = m_graph.new_node<NAWriteNode>(m_current_thread, variable, std::move(evaluation));
#ifdef TRACK_EVALUATION
  evaluation.refresh(); // Allow re-use of moved object.
#endif
  evaluation = variable;
  evaluation.add_side_effect(write_node_ptr);

  // Sequence all value-computations of the right-hand-side of an assignment before the side-effect of that assignment.
  //
  // d:before_node (must be a value-computation and not sequenced before another value-computation node)
  //     |
  //     | sb
  //     v
  // e:write_node
  //     |
  //     |              <-- in case of a prefix operator
  //     v
  //  pseudo_node       this represents the value computation of the pre-increment/decrement expression.
  //
  Evaluation const* write_evaluation = write_node_ptr.get<WriteNode>()->get_evaluation();
  Dout(dc::notice, "Generating sb edges between the rhs of an assignment (" << *write_evaluation << ") and the lhs variable " << *write_node_ptr << '.');
  DebugMarkDownRight;
  write_evaluation->for_each_node(NodeRequestedType::value_computation_heads,
      [this, &write_node_ptr, &side_effect_sb_value_computation](NodePtr const& before_node)
      {
        m_graph.new_edge(edge_sb, before_node, write_node_ptr);
        if (side_effect_sb_value_computation)
          before_node->sequenced_before_side_effect_sequenced_before_value_computation(); // Corrupt before_node to fake a (non-existing) pseudo node
                                                                                          // representing the value-computation of the pre-increment/
                                                                                          // decrement/assignment expression that write_node must be
                                                                                          // sequenced before.
      }
  COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));
  // Fake a (non-existing) pseudo node representing the value-computation after the write node.
  if (side_effect_sb_value_computation)
    write_node_ptr->sequenced_before_value_computation();
}

void Context::read(ast::tag variable, std::memory_order mo, Evaluation& evaluation)
{
  DoutTag(dc::notice, "[" << mo << " read from", variable);
  auto new_node = m_graph.new_node<AtomicReadNode>(m_current_thread, variable, mo);
  // Should be added as side effect when variable is volatile.
  evaluation.add_value_computation(new_node);
}

NodePtr Context::write(ast::tag variable, std::memory_order mo, Evaluation&& evaluation)
{
  DoutTag(dc::notice, "[" << mo << " write to", variable);
  auto write_node = m_graph.new_node<AtomicWriteNode>(m_current_thread, variable, mo, std::move(evaluation));
#ifdef TRACK_EVALUATION
  evaluation.refresh(); // Allow re-use of moved object.
#endif
  evaluation = variable;
  evaluation.add_side_effect(write_node);
  return write_node;
}

NodePtr Context::RMW(ast::tag variable, std::memory_order mo, Evaluation&& evaluation)
{
  DoutTag(dc::notice, "[" << mo << " RMW of", variable);
  auto rmw_node = m_graph.new_node<RMWNode>(m_current_thread, variable, mo, std::move(evaluation));
#ifdef TRACK_EVALUATION
  evaluation.refresh(); // Allow re-use of moved object.
#endif
  evaluation = variable;
  evaluation.add_value_computation(rmw_node);   // RMW nodes are added as value computation.
  return rmw_node;
}

NodePtr Context::compare_exchange_weak(
    ast::tag variable, ast::tag expected, int desired, std::memory_order success, std::memory_order fail, Evaluation&& evaluation)
{
  DoutTag(dc::notice, "[" << success << '/' << fail << " compare_exchange_weak of", variable);
  auto cew_node = m_graph.new_node<CEWNode>(m_current_thread, variable, expected, desired, success, fail, std::move(evaluation));
#ifdef TRACK_EVALUATION
  evaluation.refresh(); // Allow re-use of moved object.
#endif
  evaluation = variable;
  evaluation.add_value_computation(cew_node);   // CEW nodes are added as value computation.
  return cew_node;
}

Evaluation Context::lockdecl(ast::tag mutex)
{
  DoutTag(dc::notice, "[declaration of", mutex);
  auto mutex_decl_node = m_graph.new_node<MutexDeclNode>(m_current_thread, mutex);
  Evaluation result = mutex;
  result.add_side_effect(mutex_decl_node);
  return result;
}

Evaluation Context::lock(ast::tag mutex)
{
  DoutTag(dc::notice, "[lock of", mutex);
  auto mutex_node1 = m_graph.new_node<MutexReadNode>(m_current_thread, mutex);
  Evaluation result = mutex;
  result.add_value_computation(mutex_node1);
  auto mutex_node2 = m_graph.new_node<MutexLockNode>(m_current_thread, mutex);
  // Adding this as a side effect for now...
  result.add_side_effect(mutex_node2);
  m_graph.new_edge(edge_sb, mutex_node1, mutex_node2);
  return result;
}

Evaluation Context::unlock(ast::tag mutex)
{
  DoutTag(dc::notice, "[unlock of", mutex);
  auto mutex_node1 = m_graph.new_node<MutexReadNode>(m_current_thread, mutex);
  Evaluation result = mutex;
  result.add_value_computation(mutex_node1);
  auto mutex_node2 = m_graph.new_node<MutexUnlockNode>(m_current_thread, mutex);
  // Adding this as a side effect for now...
  result.add_side_effect(mutex_node2);
  m_graph.new_edge(edge_sb, mutex_node1, mutex_node2);
  return result;
}

Evaluation::node_pairs_type Context::generate_node_pairs(
    EvaluationNodes const& before_nodes,
    Evaluation const& after_evaluation
    COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel))
{
  DoutEntering(debug_channel|continued_cf, "Context::generate_node_pairs(" << before_nodes << ", " << after_evaluation << ") = ");
  Evaluation::node_pairs_type node_pairs;
  for (NodePtr const& before_node : before_nodes)
  {
    after_evaluation.for_each_node(NodeRequestedType::tails,
        [&node_pairs, &before_node](NodePtr const& after_node)
        {
          node_pairs.push_back(std::make_pair(before_node, after_node));
        }
    COMMA_DEBUG_ONLY(debug_channel));
  }
  Dout(dc::finish, node_pairs);
  return node_pairs;
}

void Context::add_edges(
    EdgeType edge_type,
    Evaluation const& before_evaluation,
    NodePtr const& after_node
    COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel))
{
  DoutEntering(debug_channel, "Context::add_edges(" << edge_type << ", " << before_evaluation << ", " << *after_node << ").");
  before_evaluation.for_each_node(NodeRequestedType::heads,
      [this, edge_type, &after_node](NodePtr const& before_node)
      {
        m_graph.new_edge(edge_type, before_node, after_node);
      }
  COMMA_DEBUG_ONLY(debug_channel));
}

void Context::add_edges(
    EdgeType edge_type,
    Evaluation::node_pairs_type node_pairs
    COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel),
    Condition const& condition)
{
#ifdef CWDEBUG
  Dout(debug_channel|continued_cf, "Generate " << edge_type << " from generated node pairs");
  if (condition.conditional())
    Dout(dc::continued, " with conditional " << condition);
  Dout(dc::finish, ".");
  DebugMarkUp;
#endif
  // Now actually add the new edges.
  for (Evaluation::node_pairs_type::iterator node_pair = node_pairs.begin(); node_pair != node_pairs.end(); ++node_pair)
    m_graph.new_edge(edge_type, node_pair->first, node_pair->second, condition);
}

void Context::add_edges(
    EdgeType edge_type,
    Evaluation const& before_evaluation,
    Evaluation const& after_evaluation
    COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel))
{
  DoutEntering(debug_channel, "Context::add_edges(" << edge_type << ", " << before_evaluation << ", " << after_evaluation << ").");
  add_edges(
      edge_type,
      generate_node_pairs(
          before_evaluation.get_nodes(NodeRequestedType::heads COMMA_DEBUG_ONLY(debug_channel)),
          after_evaluation
          COMMA_DEBUG_ONLY(debug_channel))
      COMMA_DEBUG_ONLY(debug_channel));
}

void Context::detect_full_expression_start()
{
  ++m_full_expression_detector_depth;
}

void Context::detect_full_expression_end(Evaluation& full_expression)
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

    BranchInfo* const branch_info = m_branch_info_stack.empty() ? nullptr : &m_branch_info_stack.top();

    bool previous_full_expression_is_condition = branch_info && branch_info->m_condition.conditional();

    // If m_condition is conditional then m_previous_full_expression is a condition and was moved to m_full_expression_evaluations.
    std::unique_ptr<Evaluation> const& previous_full_expression{
        previous_full_expression_is_condition ?
            m_full_expression_evaluations[branch_info->m_last_condition] :
            m_previous_full_expression};

    bool const previous_full_expression_is_valid{previous_full_expression};

#ifdef CWDEBUG
    debug::Mark* marker;
    if (previous_full_expression_is_valid)
    {
      Dout(dc::sb_edge, "Generate sequenced-before edges between " << *previous_full_expression << " and " << full_expression << ".");
      Debug(marker = new debug::Mark("\e[43;33;1m↳\e[0m"));    // DebugMarkDownRight
      s_first_full_expression = false;
    }
    else if (!s_first_full_expression)
    {
      // m_previous_full_expression should only be invalid when we encounter the first full-expression.
      // After that it should always be equal to the previous full-expression.
      DoutFatal(dc::core, "Unexpected invalid m_previous_full_expression! Last full-expression condition is " << *m_full_expression_evaluations.back());
    }
#endif
    Dout(dc::notice, "previous_full_expression_is_condition = " << previous_full_expression_is_condition);
    Dout(dc::notice, "previous_full_expression_is_valid = " << previous_full_expression_is_valid);

    // Count number of nodes in full_expression.
    int number_of_nodes = 0;
    full_expression.for_each_node(NodeRequestedType::tails, [&number_of_nodes](NodePtr const&) { ++number_of_nodes; } COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));
    Dout(dc::notice, "number_of_nodes = " << number_of_nodes);

    // Find all tail nodes in the current full_expression.
    //
    //   before_node
    //       |
    //       | sb
    //       v
    //   after_node
    //
    if (number_of_nodes > 0)
    {
      if (previous_full_expression_is_valid)
      {
        std::vector<Evaluation::node_pairs_type> skip_edge_node_pairs;
        std::vector<Condition> skip_conditions;

        Dout(dc::notice, "Size of m_finalize_branch_stack = " << m_finalize_branch_stack.size());
        while (m_finalize_branch_stack.size() > m_protected_finalize_branch_stack_size)
        {
          BranchInfo const& branch_info{m_finalize_branch_stack.top()};
          bool single_branch = branch_info.m_finalize_branch == 1;
          int index = single_branch ? branch_info.m_last_condition : branch_info.m_last_full_expression_of_previous_branch;
          Dout(dc::sb_edge, "Finalize: generate edges between between " << (single_branch ? "conditional " : "") <<
              *m_full_expression_evaluations[index] << " and " << full_expression << ".");
          DebugMarkDownRight;

          // Generate skip edge node pairs.
          skip_edge_node_pairs.emplace_back(
              generate_node_pairs(
                  m_full_expression_evaluations[index]->get_nodes(
                      NodeRequestedType::heads
                      COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge)),
                  full_expression
                  COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge)));

          // Also remember the condition to use for those.
          skip_conditions.push_back(branch_info.m_condition);

          // Pop branch info from stack.
          m_finalize_branch_stack.pop();
        }

        if (previous_full_expression_is_condition)
	{
          Dout(dc::sb_edge, "Generate edges between between conditional " << *previous_full_expression << " and " << full_expression << ".");
          DebugMarkDownRight;

          // Get all head nodes of the last full expression (which is a condition).
          EvaluationNodes previous_full_expression_nodes =
              previous_full_expression->get_nodes(NodeRequestedType::heads COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));

          add_edges(edge_sb,
              generate_node_pairs(previous_full_expression_nodes, full_expression COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge))
              COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge), branch_info->m_condition);

          branch_info->m_condition.reset();
	}
        else
          add_edges(edge_sb, *m_previous_full_expression, full_expression COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));

        // Add the skip edges.
        int i = 0;
        for (auto&& skip_edges : skip_edge_node_pairs)
          add_edges(edge_sb, skip_edges COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge), skip_conditions[i++]);
      }
      m_previous_full_expression = Evaluation::make_unique(std::move(full_expression));
      Dout(dc::notice, "SET m_previous_full_expression to full_expression (" << *m_previous_full_expression << ").");
    }

#ifdef CWDEBUG
    if (previous_full_expression_is_valid)
      delete marker;
#endif
  }
}

Context::ConditionalBranch Context::begin_branch_true()
{
  DoutEntering(dc::notice, "Context::begin_branch_with_condition()");
  ASSERT(m_previous_full_expression);
  Dout(dc::notice|continued_cf, "Adding condition from m_previous_full_expression (" << *m_previous_full_expression << ") ");
  ConditionalBranch conditional_branch{add_condition(m_previous_full_expression)};    // The previous full-expression is a conditional.
  // Create a new BranchInfo for this selection statement.
  m_branch_info_stack.emplace();
  m_branch_info_stack.top().begin_branch_true(conditional_branch, m_full_expression_evaluations, std::move(m_previous_full_expression));
  return conditional_branch;
}

void Context::BranchInfo::begin_branch_true(
    ConditionalBranch& conditional_branch,
    full_expression_evaluations_type& full_expression_evaluations,
    std::unique_ptr<Evaluation>&& previous_full_expression)
{
  m_condition = conditional_branch(true);                                             // We'll follow the true branch of this condition first.
  // We rely on this fact to know that m_previous_full_expression is a condition.
  ASSERT(m_condition.conditional());
  Dout(dc::finish, "with condition " << m_condition << '.');
  // Keep Evaluations that are conditionals alive.
  m_last_condition = full_expression_evaluations.size();
  Dout(dc::notice, "MOVING m_previous_full_expression (" << *previous_full_expression << ") to m_full_expression_evaluations!");
  full_expression_evaluations.push_back(std::move(previous_full_expression));
  m_number_of_branches = 1;
}

void Context::BranchInfo::begin_branch_false(
    ConditionalBranch& conditional_branch,
    full_expression_evaluations_type& full_expression_evaluations,
    std::unique_ptr<Evaluation>&& previous_full_expression)
{
  m_last_full_expression_of_previous_branch = full_expression_evaluations.size();
  Dout(dc::notice, "MOVING m_previous_full_expression (" << *previous_full_expression << ") to m_full_expression_evaluations!");
  full_expression_evaluations.push_back(std::move(previous_full_expression));
  m_condition = conditional_branch(false);
  // We rely on this fact to know that m_previous_full_expression is a condition.
  ASSERT(m_condition.conditional());
  m_number_of_branches = 2;
}

void Context::BranchInfo::end_branch(ConditionalBranch& conditional_branch)
{
  // If there is only one branch,
  //
  // if (condition A)         // Node 1
  // {
  //   // Node 2
  //   // Node 3
  // }
  // // Node 4
  //
  // The graph should look like this:
  //
  //             node 1
  //             | A   |
  //             v     |
  //           node 2  | _
  //             | A   | A <====
  //             v     |
  //           node 3  |
  //             | A   |
  //             v     v
  //             node 4
  //
  // while if there are two branches,
  //
  // if (condition A)         // Node 1
  // {
  //   // Node 2
  //   // Node 3
  // }
  // else
  // {
  //   // Node 4
  //   // Node 5
  // }
  // // Node 6
  //
  // the graph should look like this:
  //
  //              _node 1_  _
  //           A |        | A
  //             v        v
  //           node 2   node_4
  //           A |        | A
  //             v        v
  //           node 3   node_5
  //     ====> A |        | A
  //             v_      _v
  //               node 6
  //
  // The edge marked with the double arrow (<===) needs to be added
  // when we find the node(s) of the next full-expression (node 6).
  // The condition to be used for it is therefore:
  m_condition = conditional_branch(m_number_of_branches == 2);
  m_finalize_branch = m_number_of_branches;
  Dout(dc::notice, "m_condition = " << m_condition);
}

//static
bool Context::s_first_full_expression = true;
