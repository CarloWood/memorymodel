#include "sys.h"
#include "Context.h"
#include "Graph.h"

#ifdef CWDEBUG
// To make DoutTag work.
#define context (*this)
#endif

Evaluation Context::uninitialized(ast::tag decl)
{
  DoutTag(dc::notice, "[uninitialized declaration of", decl);
  Evaluation result;    // Uninitialzed.
  return result;
}

void Context::read(ast::tag variable, Evaluation& evaluation)
{
  DoutTag(dc::notice, "[NA read from", variable);
  auto new_node = m_graph.new_node(variable);
  // Should be added as side effect when variable is volatile.
  evaluation.add_value_computation(new_node);
}

void Context::write(ast::tag variable, Evaluation&& evaluation)
{
  DoutTag(dc::notice, "[NA write to", variable);
  auto new_node = m_graph.new_node(variable, std::move(evaluation));
#ifdef TRACK_EVALUATION
  evaluation.refresh(); // Allow re-use of moved object.
#endif
  evaluation = variable;
  evaluation.add_side_effect(new_node);

  // Sequence all value-computations of the right-hand-side of an assignment before the side-effect of that assignment.
  //
  // before_node (must be a value-computation and not sequenced before another value-computation node)
  //     |
  //     | sb
  //     v
  //  new_node
  //
  new_node->get_evaluation()->for_each_node(Node::value_computation_tails,
      [this, &new_node](Evaluation::node_iterator const& before_node)
      {
        m_graph.new_edge(before_node, new_node, edge_asw);
      }
  );
}

void Context::read(ast::tag variable, std::memory_order mo, Evaluation& evaluation)
{
  DoutTag(dc::notice, "[" << mo << " read from", variable);
  auto new_node = m_graph.new_node(variable, mo);
  // Should be added as side effect when variable is volatile.
  evaluation.add_value_computation(new_node);
}

void Context::write(ast::tag variable, std::memory_order mo, Evaluation&& evaluation)
{
  DoutTag(dc::notice, "[" << mo << " write to", variable);
  auto new_node = m_graph.new_node(variable, mo, std::move(evaluation));
#ifdef TRACK_EVALUATION
  evaluation.refresh(); // Allow re-use of moved object.
#endif
  evaluation = variable;
  evaluation.add_side_effect(new_node);
}

Evaluation Context::lockdecl(ast::tag mutex)
{
  DoutTag(dc::notice, "[declaration of", mutex);
  auto mutex_decl_node = m_graph.new_node(mutex, mutex_decl);
  Evaluation result = mutex;
  result.add_side_effect(mutex_decl_node);
  return result;
}

Evaluation Context::lock(ast::tag mutex)
{
  DoutTag(dc::notice, "[lock of", mutex);
  auto mutex_node1 = m_graph.new_node(mutex, mutex_lock1);
  Evaluation result = mutex;
  result.add_value_computation(mutex_node1);
  auto mutex_node2 = m_graph.new_node(mutex, mutex_lock2);
  // Adding this as a side effect for now...
  result.add_side_effect(mutex_node2);
  return result;
}

Evaluation Context::unlock(ast::tag mutex)
{
  DoutTag(dc::notice, "[unlock of", mutex);
  auto mutex_node1 = m_graph.new_node(mutex, mutex_unlock1);
  Evaluation result = mutex;
  result.add_value_computation(mutex_node1);
  auto mutex_node2 = m_graph.new_node(mutex, mutex_unlock2);
  // Adding this as a side effect for now...
  result.add_side_effect(mutex_node2);
  return result;
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
    Dout(dc::notice, "FULL-EXPRESSION FOUND: " << full_expression);
    // An Evaluation is allocated iff when the expression is pointed to by a std::unique_ptr,
    // which are only used as member functions of another Evaluation; hence a full-expression
    // which can never be a part of another (full-)expression, will never be allocated.
    ASSERT(!full_expression.is_allocated());

    if (m_last_full_expression.is_valid())
      Dout(dc::notice, "Generate sequenced-before edges between " << m_last_full_expression << " and " << full_expression << ".");

    int number_of_nodes = 0;
    // Find all nodes in the current full_expression.
    //
    //   before_node
    //       |
    //       | sb
    //       v
    //   after_node
    //
    full_expression.for_each_node(Node::heads,
        [this, &number_of_nodes](Evaluation::node_iterator const& after_node)
        {
          ++number_of_nodes;
          // Generate all sequenced-before edges between full-expressions.
          if (m_last_full_expression.is_valid())
          {
            m_last_full_expression.for_each_node(Node::tails,
                [this, &after_node](Evaluation::node_iterator const& before_node)
                {
                  m_graph.new_edge(before_node, after_node, edge_sb);
                }
            );
          }
        }
    );

    // Replace m_last_full_expression with the current one if there was any node at all.
    if (number_of_nodes > 0)
    {
      m_last_full_expression = std::move(full_expression);
    }
  }
}
