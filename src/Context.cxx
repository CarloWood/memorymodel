#include "sys.h"
#include "Context.h"
#include "Graph.h"

#ifdef CWDEBUG
// To make DoutTag work.
#define context (*this)
#endif

void Context::scope_start(bool is_thread)
{
  DoutEntering(dc::notice, "Context::scope_start('is_thread' = " << is_thread << ")");
  m_threads.push(is_thread);
  if (is_thread)
  {
    m_beginning_of_thread = true;
    m_current_thread = Thread::create_new_thread(m_next_thread_id, m_current_thread);
    m_last_full_expressions.emplace(std::move(m_last_full_expression));
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
    m_last_full_expression = std::move(m_last_full_expressions.top());
    m_last_full_expressions.pop();
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
  auto new_node = m_graph.new_node(m_current_thread, variable);
  // Should be added as side effect when variable is volatile.
  evaluation.add_value_computation(new_node);
}

void Context::write(ast::tag variable, Evaluation&& evaluation, bool side_effect_sb_value_computation)
{
  DoutTag(dc::notice, "[NA write to", variable);
  auto write_node = m_graph.new_node(m_current_thread, variable, std::move(evaluation));
#ifdef TRACK_EVALUATION
  evaluation.refresh(); // Allow re-use of moved object.
#endif
  evaluation = variable;
  evaluation.add_side_effect(write_node);

  // Sequence all value-computations of the right-hand-side of an assignment before the side-effect of that assignment.
  //
  // before_node (must be a value-computation and not sequenced before another value-computation node)
  //     |
  //     | sb
  //     v
  //  write_node
  //     |
  //     |              <-- in case of a prefix operator
  //     v
  //  pseudo_node       this represents the value computation of the pre-increment/decrement expression.
  //
  Dout(dc::notice, "Generating sb edges between the rhs of an assignment (" << *write_node->get_evaluation() << ") and the lhs variable " << *write_node << '.');
  DebugMarkDownRight;
  write_node->get_evaluation()->for_each_node(Node::value_computation_heads,
      [this, &write_node, &side_effect_sb_value_computation](Evaluation::node_iterator const& before_node)
      {
        m_graph.new_edge(edge_sb, before_node, write_node);
        if (side_effect_sb_value_computation)
          before_node->sequenced_before_side_effect_sequenced_before_value_computation(); // Corrupt before_node to fake a (non-existing) pseudo node
                                                                                          // representing the value-computation of the pre-increment/
                                                                                          // decrement/assignment expression that write_node must be
                                                                                          // sequenced before.
      }
  COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));
  // Fake a (non-existing) pseudo node representing the value-computation after the write_node.
  if (side_effect_sb_value_computation)
    write_node->sequenced_before_value_computation();
}

void Context::read(ast::tag variable, std::memory_order mo, Evaluation& evaluation)
{
  DoutTag(dc::notice, "[" << mo << " read from", variable);
  auto new_node = m_graph.new_node(m_current_thread, variable, mo);
  // Should be added as side effect when variable is volatile.
  evaluation.add_value_computation(new_node);
}

Evaluation::node_iterator Context::write(ast::tag variable, std::memory_order mo, Evaluation&& evaluation)
{
  DoutTag(dc::notice, "[" << mo << " write to", variable);
  auto write_node = m_graph.new_node(m_current_thread, variable, mo, std::move(evaluation));
#ifdef TRACK_EVALUATION
  evaluation.refresh(); // Allow re-use of moved object.
#endif
  evaluation = variable;
  evaluation.add_side_effect(write_node);
  return write_node;
}

Evaluation Context::lockdecl(ast::tag mutex)
{
  DoutTag(dc::notice, "[declaration of", mutex);
  auto mutex_decl_node = m_graph.new_node(m_current_thread, mutex, mutex_decl);
  Evaluation result = mutex;
  result.add_side_effect(mutex_decl_node);
  return result;
}

Evaluation Context::lock(ast::tag mutex)
{
  DoutTag(dc::notice, "[lock of", mutex);
  auto mutex_node1 = m_graph.new_node(m_current_thread, mutex, mutex_lock1);
  Evaluation result = mutex;
  result.add_value_computation(mutex_node1);
  auto mutex_node2 = m_graph.new_node(m_current_thread, mutex, mutex_lock2);
  // Adding this as a side effect for now...
  result.add_side_effect(mutex_node2);
  m_graph.new_edge(edge_sb, mutex_node1, mutex_node2);
  return result;
}

Evaluation Context::unlock(ast::tag mutex)
{
  DoutTag(dc::notice, "[unlock of", mutex);
  auto mutex_node1 = m_graph.new_node(m_current_thread, mutex, mutex_unlock1);
  Evaluation result = mutex;
  result.add_value_computation(mutex_node1);
  auto mutex_node2 = m_graph.new_node(m_current_thread, mutex, mutex_unlock2);
  // Adding this as a side effect for now...
  result.add_side_effect(mutex_node2);
  m_graph.new_edge(edge_sb, mutex_node1, mutex_node2);
  return result;
}

Evaluation::node_pairs_type Context::generate_node_pairs(
    Evaluation const& before_evaluation,
    Evaluation const& after_evaluation
    COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel)) const
{
  Dout(debug_channel, "Generate node pairs for edges between between " << before_evaluation << " and " << after_evaluation << ".");
  DebugMarkDownRight;

  // First find all new edges without actually adding new ones (that would interfere with the algorithm).
  Evaluation::node_pairs_type node_pairs;

  // Find all tail nodes in after_evaluation.
  after_evaluation.for_each_node(Node::tails,
      [&before_evaluation, &node_pairs COMMA_DEBUG_ONLY(&debug_channel)](Evaluation::node_iterator const& after_node)
      {
        // Find all node pairs that need a new edge.
        before_evaluation.for_each_node(Node::heads,
            [&after_node, &node_pairs](Evaluation::node_iterator const& before_node)
            {
              node_pairs.push_back(std::make_pair(before_node, after_node));
            }
        COMMA_DEBUG_ONLY(debug_channel));
      }
  COMMA_DEBUG_ONLY(debug_channel));

  return node_pairs;
}

void Context::add_edges(EdgeType edge_type, Evaluation::node_pairs_type node_pairs COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel))
{
  Dout(debug_channel, "Generate " << edge_type << " from generated node pairs.");
  DebugMarkUp;
  // Now actually add the new edges.
  for (Evaluation::node_pairs_type::iterator node_pair = node_pairs.begin(); node_pair != node_pairs.end(); ++node_pair)
    m_graph.new_edge(edge_type, node_pair->first, node_pair->second);
}

void Context::add_edges(
    EdgeType edge_type,
    Evaluation const& before_evaluation,
    Evaluation const& after_evaluation
    COMMA_DEBUG_ONLY(libcwd::channel_ct& debug_channel))
{
  add_edges(edge_type, generate_node_pairs(before_evaluation, after_evaluation COMMA_DEBUG_ONLY(debug_channel)) COMMA_DEBUG_ONLY(debug_channel));
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

#ifdef CWDEBUG
    debug::Mark* marker;
#endif
    bool last_full_expression_is_valid = m_last_full_expression.is_valid();
    if (last_full_expression_is_valid)
    {
      Dout(dc::sb_edge, "Generate sequenced-before edges between " << m_last_full_expression << " and " << full_expression << ".");
      Debug(marker = new debug::Mark("\e[43;33;1m↳\e[0m"));    // DebugMarkDownRight
    }

    // First find all new edges without actually adding new ones (that would interfere with the algorithm).
    Evaluation::node_pairs_type node_pairs;
    int number_of_nodes = 0;
    // Find all tail nodes in the current full_expression.
    //
    //   before_node
    //       |
    //       | sb
    //       v
    //   after_node
    //
    full_expression.for_each_node(Node::tails,
        [this, &number_of_nodes, &node_pairs](Evaluation::node_iterator const& after_node)
        {
          ++number_of_nodes;
          // Generate all sequenced-before edges between full-expressions.
          if (m_last_full_expression.is_valid())
          {
            m_last_full_expression.for_each_node(Node::heads,
                [this, &after_node, &node_pairs](Evaluation::node_iterator const& before_node)
                {
                  node_pairs.push_back(std::make_pair(before_node, after_node));
                }
            COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));
          }
        }
    COMMA_DEBUG_ONLY(DEBUGCHANNELS::dc::sb_edge));
    // Now actually add the new edges.
    for (Evaluation::node_pairs_type::iterator node_pair = node_pairs.begin(); node_pair != node_pairs.end(); ++node_pair)
      m_graph.new_edge(edge_sb, node_pair->first, node_pair->second);

#ifdef CWDEBUG
    if (last_full_expression_is_valid)
      delete marker;
#endif

    // Replace m_last_full_expression with the current one if there was any node at all.
    if (number_of_nodes > 0)
    {
      m_last_full_expression = std::move(full_expression);
    }
  }
}
