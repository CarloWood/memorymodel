#include "sys.h"
#include "Locks.h"
#include "Context.h"

void Locks::left_scope(ast::unique_lock_decl const& unique_lock_decl, Context& context)
{
  context.unlock(unique_lock_decl.m_mutex);
}
