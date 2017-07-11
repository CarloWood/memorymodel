#include "sys.h"
#include "Context.h"
#include "Graph.h"

#ifdef CWDEBUG
// To make DoutTag work.
#define context (*this)
#endif

void Context::uninitialized(ast::tag decl)
{
  DoutTag(dc::notice, "[uninitialized declaration of", decl);
}

void Context::read(ast::tag variable)
{
  DoutTag(dc::notice, "[NA read from", variable);
  auto node = m_graph.new_node(variable);
  // Should be added as side effect when variable is volatile.
  add_value_computation(node);
}

void Context::write(ast::tag variable, Evaluation&& evaluation)
{
  DoutTag(dc::notice, "[NA write to", variable);
  auto node = m_graph.new_node(variable, std::move(evaluation));
  add_side_effect(node);
  evaluation = variable;
}

void Context::read(ast::tag variable, std::memory_order mo)
{
  DoutTag(dc::notice, "[" << mo << " read from", variable);
  auto node = m_graph.new_node(variable, mo);
  // Should be added as side effect when variable is volatile.
  add_value_computation(node);
}

void Context::write(ast::tag variable, std::memory_order mo, Evaluation&& evaluation)
{
  DoutTag(dc::notice, "[" << mo << " write to", variable);
  auto node = m_graph.new_node(variable, mo, std::move(evaluation));
  add_side_effect(node);
  evaluation = variable;
}

void Context::lockdecl(ast::tag mutex)
{
  DoutTag(dc::notice, "[declaration of", mutex);
  auto node = m_graph.new_node(mutex, mutex_decl);
  add_side_effect(node);
}

void Context::lock(ast::tag mutex)
{
  DoutTag(dc::notice, "[lock of", mutex);
  auto node = m_graph.new_node(mutex, mutex_lock1);
  add_value_computation(node);
  auto mutex_node = m_graph.new_node(mutex, mutex_lock2);
  // Adding this as a side effect for now...
  add_side_effect(mutex_node);
}

void Context::unlock(ast::tag mutex)
{
  DoutTag(dc::notice, "[unlock of", mutex);
  auto node = m_graph.new_node(mutex, mutex_unlock1);
  add_value_computation(node);
  auto mutex_node = m_graph.new_node(mutex, mutex_unlock2);
  // Adding this as a side effect for now...
  add_side_effect(mutex_node);
}

void Context::sequence_point()
{
  DoutEntering(dc::notice, "Context::sequence_point()");
  ++m_current_sequence_point_id;
}

void Context::detect_full_expression_start()
{
  ++m_full_expression_detector_depth;
}

void Context::detect_full_expression_end()
{
  // An expression that is not part of another full-expression is a full-expression.
  if (--m_full_expression_detector_depth == 0)
  {
    // http://en.cppreference.com/w/cpp/language/eval_order
    // 1) There is a sequence point at the end of each full expression.
    sequence_point();
  }
}
