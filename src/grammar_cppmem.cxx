#include "sys.h"
#include "grammar_cppmem.h"
#include "position_handler.h"
#include "SymbolsImpl.h"
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#ifdef BOOST_SPIRIT_QI_DEBUG
#include <boost/tuple/tuple_io.hpp>
#endif

BOOST_FUSION_ADAPT_STRUCT(ast::memory_location, m_name)
BOOST_FUSION_ADAPT_STRUCT(ast::vardecl, m_type, m_memory_location, m_initial_value)
BOOST_FUSION_ADAPT_STRUCT(ast::mutex_decl, m_name)
BOOST_FUSION_ADAPT_STRUCT(ast::condition_variable_decl, m_name)
BOOST_FUSION_ADAPT_STRUCT(ast::unique_lock_decl, m_name, m_mutex)
BOOST_FUSION_ADAPT_STRUCT(ast::statement, m_statement_node, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::load_call, m_memory_location_id, m_memory_order, m_readsvalue)
BOOST_FUSION_ADAPT_STRUCT(ast::store_call, m_memory_location_id, m_val, m_memory_order)
BOOST_FUSION_ADAPT_STRUCT(ast::register_assignment, lhs, rhs)
BOOST_FUSION_ADAPT_STRUCT(ast::assignment, lhs, rhs)

BOOST_FUSION_ADAPT_STRUCT(ast::postfix_expression, m_postfix_expression_node, m_postfix_operators)
BOOST_FUSION_ADAPT_STRUCT(ast::unary_expression, m_unary_operators, m_postfix_expression)
BOOST_FUSION_ADAPT_STRUCT(ast::multiplicative_expression, m_other_expression, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::additive_expression, m_other_expression, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::shift_expression, m_other_expression, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::relational_expression, m_other_expression, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::equality_expression, m_other_expression, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::and_expression, m_other_expression, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::exclusive_or_expression, m_other_expression, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::inclusive_or_expression, m_other_expression, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::logical_and_expression, m_other_expression, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::logical_or_expression, m_other_expression, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::conditional_expression, m_logical_or_expression, m_conditional_expression_tail)
BOOST_FUSION_ADAPT_STRUCT(ast::assignment_expression, m_assignment_expression_node, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::primary_expression, m_primary_expression_node, m_dummy)

BOOST_FUSION_ADAPT_STRUCT(ast::expression, m_assignment_expression, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::function_call, m_function, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::atomic_fetch_add_explicit, m_memory_location_id, m_expression, m_memory_order)
BOOST_FUSION_ADAPT_STRUCT(ast::atomic_fetch_sub_explicit, m_memory_location_id, m_expression, m_memory_order)
BOOST_FUSION_ADAPT_STRUCT(ast::atomic_compare_exchange_weak_explicit, m_memory_location_id, m_expected, m_desired, m_succeed, m_fail)
BOOST_FUSION_ADAPT_STRUCT(ast::function, m_function_name, m_compound_statement)
BOOST_FUSION_ADAPT_STRUCT(ast::statement_seq, m_statements, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::compound_statement, m_statement_seq)
BOOST_FUSION_ADAPT_STRUCT(ast::threads, m_threads, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::if_statement, m_condition, m_then, m_else)
BOOST_FUSION_ADAPT_STRUCT(ast::while_statement, m_condition, m_statement)
BOOST_FUSION_ADAPT_STRUCT(ast::break_statement, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::wait_call, m_condition_variable, m_unique_lock, m_compound_statement)
BOOST_FUSION_ADAPT_STRUCT(ast::notify_all_call, m_condition_variable, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::mutex_lock_call, m_mutex, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::mutex_unlock_call, m_mutex, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::return_statement, m_expression)
BOOST_FUSION_ADAPT_STRUCT(ast::declaration_statement, m_declaration_statement_node)
BOOST_FUSION_ADAPT_STRUCT(ast::jump_statement, m_jump_statement_node)
BOOST_FUSION_ADAPT_STRUCT(ast::iteration_statement, m_while_statement)
BOOST_FUSION_ADAPT_STRUCT(ast::selection_statement, m_if_statement)
BOOST_FUSION_ADAPT_STRUCT(ast::expression_statement, m_expression)

namespace parser {

namespace phoenix = boost::phoenix;

//=====================================
// Cppmem program grammar
//=====================================

template<typename Iterator>
grammar_cppmem<Iterator>::grammar_cppmem(position_handler<Iterator>& handler) :
    grammar_cppmem::base_type(cppmem, "grammar_cppmem")
{
  auto& na_memory_locations{Symbols::instance().m_impl->na_memory_locations};
  auto& atomic_memory_locations{Symbols::instance().m_impl->atomic_memory_locations};
  auto& register_locations{Symbols::instance().m_impl->register_locations};
  auto& function_names{Symbols::instance().m_impl->function_names};
  auto& mutexes{Symbols::instance().m_impl->mutexes};
  auto& condition_variables{Symbols::instance().m_impl->condition_variables};
  auto& unique_locks{Symbols::instance().m_impl->unique_locks};
  using namespace qi;
  attr_type dummy;

  memory_order.add
      ("std::memory_order_relaxed", std::memory_order_relaxed)
      ("memory_order_relaxed", std::memory_order_relaxed)
      ("mo_relaxed", std::memory_order_relaxed)
      ("std::memory_order_consume", std::memory_order_consume)
      ("memory_order_consume", std::memory_order_consume)
      ("mo_consume", std::memory_order_consume)
      ("std::memory_order_acquire", std::memory_order_acquire)
      ("memory_order_acquire", std::memory_order_acquire)
      ("mo_acquire", std::memory_order_acquire)
      ("std::memory_order_release", std::memory_order_release)
      ("memory_order_release", std::memory_order_release)
      ("mo_release", std::memory_order_release)
      ("std::memory_order_acq_rel", std::memory_order_acq_rel)
      ("memory_order_acq_rel", std::memory_order_acq_rel)
      ("mo_acq_rel", std::memory_order_acq_rel)
      ("std::memory_order_seq_cst", std::memory_order_seq_cst)
      ("memory_order_seq_cst", std::memory_order_seq_cst)
      ("mo_seq_cst", std::memory_order_seq_cst)
  ;

  equality_operator.add
      ("==", ast::eo_eq)
      ("!=", ast::eo_ne)
  ;

  relational_operator.add
      ("<",  ast::ro_lt)
      (">",  ast::ro_gt)
      (">=", ast::ro_ge)
      ("<=", ast::ro_le)
  ;

  shift_operator.add
      ("<<", ast::so_shl)
      (">>", ast::so_shr)
  ;

  additive_operator.add
      ("+", ast::ado_add)
      ("-", ast::ado_sub)
  ;

  multiplicative_operator.add
      ("*", ast::mo_mul)
      ("/", ast::mo_div)
  ;

  unary_operator.add
      ("++", ast::uo_inc)       // Not really a unary operator ;)
      ("--", ast::uo_dec)       // Same.
      ("*", ast::uo_dereference)
      ("&", ast::uo_reference)
      ("+", ast::uo_plus)
      ("-", ast::uo_minus)
      ("!", ast::uo_not)
      ("~", ast::uo_invert)
  ;

  postfix_operator.add
      ("++", ast::po_inc)
      ("--", ast::po_dec)
  ;

  atomic_fetch_add_explicit =
      -lit("std::") >> "atomic_fetch_add_explicit" > '(' > '&' > atomic_memory_locations > ',' > assignment_expression > ',' > memory_order > ')';

  atomic_fetch_sub_explicit =
      -lit("std::") >> "atomic_fetch_sub_explicit" > '(' > '&' > atomic_memory_locations > ',' > assignment_expression > ',' > memory_order > ')';

  atomic_compare_exchange_weak_explicit =
      -lit("std::") >> "atomic_compare_exchange_weak_explicit" > '(' >
          '&' > atomic_memory_locations > ',' > int_ > ',' > int_ > ',' > memory_order > ',' > memory_order > ')';

  register_location =
      lexeme[ 'r' >> uint_ >> !(alnum | char_('_')) ];

  register_assignment =
      register_location >> lexeme['=' >> !char_('=')] > assignment_expression;   // Not supporting braced-init-list.

  assignment =
      na_memory_locations >> lexeme['=' >> !char_('=')] > assignment_expression; // Not supporting braced-init-list.

  load_call =
      atomic_memory_locations >> '.' >> "load" >> '(' >> (memory_order | attr(std::memory_order_seq_cst)) >> ')' >> -(".readsvalue(" >> int_ >> ')');

  store_call =
      atomic_memory_locations >> '.' >> "store" >> '(' >> assignment_expression >> ((',' >> memory_order) | attr(std::memory_order_seq_cst)) >> ')';

  function_call =
      function_names >> '(' >> ')' >> dummy(false);

  jump_statement =
    ( break_statement
    | return_statement
    );

  break_statement =
      "break" > ';' >> dummy(false);

  return_statement =
      "return" > expression > ';';

  wait_call =
      (condition_variables > lit('.')) >> "wait" > '(' > unique_locks > ',' > "[&]" > compound_statement > ')';

  notify_all_call =
      (condition_variables > lit('.')) >> "notify_all" > '(' > ')' > dummy(false);

  mutex_lock_call =
      (mutexes > lit('.')) >> "lock" > '(' > ')' > dummy(false);

  mutex_unlock_call =
      (mutexes > lit('.')) >> "unlock" > '(' > ')' > dummy(false);

  expression_statement =
      (expression > ';') | ';';

  postfix_expression =
    ( primary_expression
    | atomic_fetch_add_explicit                 // Not supporting a long list of stuff,
    | atomic_fetch_sub_explicit                 // see http://www.nongnu.org/hcb/#postfix-expression.
    | atomic_compare_exchange_weak_explicit     // These are the function calls we have.
    | load_call                                 //
    ) >> *postfix_operator;

  unary_expression =
      *unary_operator >> postfix_expression;    // Short cut from 'postfix-expression | cast-expression', since we're not supporting casts.

#if 0
  cast_expression =
    unary_expression; // Not supporting casting.

  pm_expression =
    cast_expression;  // Not supporting member function pointers.
#endif

  multiplicative_expression =
      /*pm_expression*/ unary_expression >> *(multiplicative_operator > /*pm_expression*/ unary_expression);

  additive_expression =
      multiplicative_expression >> *(additive_operator > multiplicative_expression);

  shift_expression =
      additive_expression >> *(shift_operator > additive_expression);

  relational_expression =
      shift_expression >> *(relational_operator > shift_expression);

  equality_expression =
      relational_expression >> *(equality_operator > relational_expression);

  and_expression =
      equality_expression >> *(lexeme[ '&' >> !lit('&') ] > equality_expression);

  exclusive_or_expression =
      and_expression >> *('^' > and_expression);

  inclusive_or_expression =
      exclusive_or_expression >> *(lexeme[ '|' >> !lit('|') ] > exclusive_or_expression);

  logical_and_expression =
      inclusive_or_expression >> *("&&" > inclusive_or_expression);

  logical_or_expression =
      logical_and_expression >> *("||" > logical_and_expression);

  conditional_expression =
      logical_or_expression >> -('?' > expression > ':' > assignment_expression);

  assignment_expression =
    ( register_assignment       // Try assignments first (they start with a primary_expression that also matches a conditional_expression).
    | assignment
    | conditional_expression
    // | logical_or_expression >> assignment_operator > initializer_clause
    // We do not support assigning to overloaded binary operators, only to registers and variables:
    ) >> dummy(false);

  expression =
      assignment_expression >> *(',' > assignment_expression);

  primary_expression =
    ( ('(' > expression > ')')
    | int_
    | bool_
    | register_locations
    | na_memory_locations
    ) >> dummy(false);

  statement =
    ( expression_statement
    | (store_call > ';')
    | (function_call > ';')
    | (wait_call > ';')
    | (notify_all_call > ';')
    | (mutex_lock_call > ';')
    | (mutex_unlock_call > ';')
    | threads
    | compound_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    | declaration_statement
    ) >> dummy(false);

  statement_seq =
      +statement >> dummy(false);

  // Unused_type rules with on_success actions.
  scope_begin                 = (lit('{') - "{{{");
  scope_end                   = lit('}');
  threads_begin               = lit("{{{");
  threads_next                = lit("|||");
  threads_end                 = lit("}}}");

  // int main() { ... [return 0;] }   // the optional return statement is ignored.
  main =
      "int" > no_skip[whitespace] > string("main") > "()" > compound_statement;

  selection_statement =
      if_statement;

  else_statement =
      lit("else") > statement;

  if_statement =
      lit("if") >> '(' >> expression >> ')' >> statement >> -else_statement;

  iteration_statement =
      while_statement;

  while_statement =
      lit("while") >> '(' >> expression >> ')' > statement;

  // void function_name() { ... }
  function =
      "void" > no_skip[whitespace] > function_name > "()" > compound_statement;

  function_name =
      identifier;

  compound_statement =
      scope_begin > -statement_seq > scope_end;

  threads =
      threads_begin > statement_seq >> +(threads_next > statement_seq) > threads_end >> dummy(false);

  cppmem =
      *(declaration_statement | function) > main;

  // No-skipper rules.
  identifier_begin_char = alpha | char_('_');
  identifier_char       = alnum | char_('_');

  // A type.
  type =
      ( "bool"                        >> attr(ast::type_bool)
      | "int"                         >> attr(ast::type_int)
      | -lit("std::") >> "atomic_int" >> attr(ast::type_atomic_int)
      ) >> !no_skip[ identifier_char ];

  // The name of a variable in memory, atomic or non-atomic.
  memory_location =
      identifier - register_location;

  // The name of a register variable name (not in memory).
  register_location =
      lexeme[ 'r' >> uint_ >> !identifier_char ];

  // Allowable variable and function names.
  identifier =
      lexeme[ identifier_begin_char >> *identifier_char ];

  declaration_statement =
      ( mutex_decl
      | condition_variable_decl
      | unique_lock_decl
      | vardecl
      );

  // Variable declaration (global or inside a scope).
  vardecl =
      type >> no_skip[whitespace] >> memory_location >> -('=' > expression) >> ';';

  mutex_decl =
      -lit("std::") >> "mutex" >> no_skip[whitespace] >> identifier > ';';

  condition_variable_decl =
      -lit("std::") >> "condition_variable" >> no_skip[whitespace] >> identifier > ';';

  unique_lock_decl =
      -lit("std::") >> "unique_lock" > '<' >> -lit("std::") > "mutex" > '>' > identifier > '(' > mutexes > ')' > ';';

  // Debugging and error handling and reporting support.
  using qi::debug;
  BOOST_SPIRIT_DEBUG_NODES(
      //(identifier_begin_char)
      //(identifier_char)
      (type)
      (memory_location)
      (register_location)
      (identifier)
      (vardecl)
      (mutex_decl)
      (condition_variable_decl)
      (unique_lock_decl)
      (scope_begin)
      (scope_end)
      (threads_begin)
      (threads_next)
      (threads_end)
      (main)
      (function)
      (function_name)
      (compound_statement)
      (statement_seq)
      (threads)
      (cppmem)
      (statement)
      (conditional_expression)
      (postfix_expression)
      //(cast_expression)
      //(pm_expression)
      (multiplicative_expression)
      (additive_expression)
      (shift_expression)
      (relational_expression)
      (equality_expression)
      (and_expression)
      (exclusive_or_expression)
      (inclusive_or_expression)
      (logical_and_expression)
      (logical_or_expression)
      (assignment_expression)
      (primary_expression)
      (expression)
      (register_assignment)
      (assignment)
      (atomic_fetch_add_explicit)
      (atomic_fetch_sub_explicit)
      (atomic_compare_exchange_weak_explicit)
      (load_call)
      (store_call)
      (register_location)
      (function_call)
      (wait_call)
      (notify_all_call)
      (if_statement)
      (while_statement)
      (jump_statement)
      (expression_statement)
      (selection_statement)
      (iteration_statement)
      (compound_statement)
      (break_statement)
      (return_statement)
  );

  using handler_function = phoenix::function<position_handler<Iterator>>;

  // Error handling: on error in start, call handler.
  on_error<fail>
  (
      cppmem
    , handler_function(handler)("Error! Expecting ", _4, _3)
  );

  // Error handling: on error in start, call handler.
  on_error<fail>
  (
      statement
    , handler_function(handler)("Error! Expecting ", _4, _3)
  );

  // Annotation: on success in register_assignment, call position_handler.
  on_success(
      register_assignment
    , handler_function(handler)(_val, _1)
  );

  on_success(
      vardecl
    , handler_function(handler)(_val, _1)
  );

  on_success(
      mutex_decl
    , handler_function(handler)(_val, _1)
  );

  on_success(
      condition_variable_decl
    , handler_function(handler)(_val, _1)
  );

  on_success(
      unique_lock_decl
    , handler_function(handler)(_val, _1)
  );

  on_success(
      function
    , handler_function(handler)(_val, _1)
  );

  on_success(
      main
    , handler_function(handler)(_val, _1)
  );

  on_success(
      scope_begin
    , handler_function(handler)(1, _1)
  );

  on_success(
      scope_end
    , handler_function(handler)(0, _1)
  );

  on_success(
      threads_begin
    , handler_function(handler)(1, _1)
  );

  on_success(
      threads_next
    , handler_function(handler)(2, _1)
  );

  on_success(
      threads_end
    , handler_function(handler)(0, _1)
  );
}

} // namespace parser

// Instantiate grammar template.
template class parser::grammar_cppmem<std::string::const_iterator>;
