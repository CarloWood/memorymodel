#include "sys.h"
#include "Locks.h"
#include "Context.h"

void Locks::left_scope(ast::unique_lock_decl const& unique_lock_decl, Context& context)
{
  // As this is the result of the invocation of the destructor generated at the end of
  // the lifetime of an std::unique_lock<std::mutex>, this should be treated as a
  // full-expression as per http://eel.is/c++draft/intro.execution#12.4.
  Evaluation full_expression;
  FullExpressionDetector detector(full_expression, context);
  full_expression = context.unlock(unique_lock_decl.m_mutex);
}
