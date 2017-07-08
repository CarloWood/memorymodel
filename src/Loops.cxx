#include "sys.h"
#include "debug.h"
#include "Loops.h"

void Loops::enter(ast::iteration_statement const& iteration_statement)
{
  m_stack.push(iteration_statement);
}

void Loops::leave(ast::iteration_statement const& iteration_statement)
{
  DebugMarkUp;
  // This is where a break jumps to.
  Dout(dc::notice, "TODO: left scope of: " << iteration_statement);
  m_stack.pop();
}

void Loops::add_break(ast::break_statement const& break_statement)
{
  Dout(dc::notice, "TODO: break;");
}

