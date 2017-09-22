#include "sys.h"
#include "debug.h"

#ifdef CWDEBUG
#include "Evaluation.h"
#include "ast.h"
#include "boolean-expression/BooleanExpression.h"

std::ostream& operator<<(std::ostream& os, std::memory_order mo)
{
  switch (mo)
  {
    case std::memory_order_relaxed:
      os << "memory_order_relaxed";
      break;
    case std::memory_order_consume:
      os << "memory_order_consume";
      break;
    case std::memory_order_acquire:
      os << "memory_order_acquire";
      break;
    case std::memory_order_release:
      os << "memory_order_release";
      break;
    case std::memory_order_acq_rel:
      os << "memory_order_acq_rel";
      break;
    case std::memory_order_seq_cst:
      os << "memory_order_seq_cst";
      break;
  }
  return os;
}

void gdb_print_evaluation(Evaluation const&& evaluation)
{
  std::cout << evaluation << std::endl;
}

void gdb_print_expression(ast::expression const&& expression)
{
  std::cout << expression << std::endl;
}

void gdb_print_boolean_expression(boolean::Expression const&& expression)
{
  std::cout << expression << std::endl;
}
#endif // CWDEBUG
