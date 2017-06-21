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

BOOST_FUSION_ADAPT_STRUCT(ast::memory_location, m_name)
BOOST_FUSION_ADAPT_STRUCT(ast::vardecl, m_type, m_memory_location, m_initial_value)
BOOST_FUSION_ADAPT_STRUCT(ast::mutex_decl, m_name);
BOOST_FUSION_ADAPT_STRUCT(ast::condition_variable_decl, m_name);
BOOST_FUSION_ADAPT_STRUCT(ast::unique_lock_decl, m_name, m_mutex);
BOOST_FUSION_ADAPT_STRUCT(ast::statement, m_statement)
BOOST_FUSION_ADAPT_STRUCT(ast::load_statement, m_memory_location_id, m_memory_order, m_readsvalue)
BOOST_FUSION_ADAPT_STRUCT(ast::store_statement, m_memory_location_id, m_val, m_memory_order)
BOOST_FUSION_ADAPT_STRUCT(ast::register_assignment, lhs, rhs)
BOOST_FUSION_ADAPT_STRUCT(ast::assignment, lhs, rhs)
BOOST_FUSION_ADAPT_STRUCT(ast::simple_expression, m_simple_expression_node)
BOOST_FUSION_ADAPT_STRUCT(ast::unary_expression, m_negated, m_simple_expression)
BOOST_FUSION_ADAPT_STRUCT(ast::chain, op, operand)
BOOST_FUSION_ADAPT_STRUCT(ast::expression, m_operand, m_chained)
BOOST_FUSION_ADAPT_STRUCT(ast::function_call, m_function)
BOOST_FUSION_ADAPT_STRUCT(ast::atomic_fetch_add_explicit, m_memory_location_id, m_expression, m_memory_order);
BOOST_FUSION_ADAPT_STRUCT(ast::atomic_fetch_sub_explicit, m_memory_location_id, m_expression, m_memory_order);
BOOST_FUSION_ADAPT_STRUCT(ast::atomic_compare_exchange_weak_explicit, m_memory_location_id, m_expected, m_desired, m_succeed, m_fail);
BOOST_FUSION_ADAPT_STRUCT(ast::function, m_function_name, m_scope)
BOOST_FUSION_ADAPT_STRUCT(ast::body, m_body_nodes, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::scope, m_body)
BOOST_FUSION_ADAPT_STRUCT(ast::threads, m_threads, m_dummy)
BOOST_FUSION_ADAPT_STRUCT(ast::if_statement, m_condition, m_then, m_else);
BOOST_FUSION_ADAPT_STRUCT(ast::while_statement, m_condition, m_body);
BOOST_FUSION_ADAPT_STRUCT(ast::break_statement, m_dummy);
BOOST_FUSION_ADAPT_STRUCT(ast::wait_statement, m_condition_variable, m_unique_lock, m_scope);
BOOST_FUSION_ADAPT_STRUCT(ast::return_statement, m_expression);
BOOST_FUSION_ADAPT_STRUCT(ast::declaration_statement, m_declaration_statement_node);

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

  operators.add
      ("==", ast::op_eq)
      ("!=", ast::op_ne)
      ("<",  ast::op_lt)
      (">",  ast::op_gt)
      (">=", ast::op_ge)
      ("<=", ast::op_le)
      ("||", ast::op_bo)
      ("&&", ast::op_ba)
  ;

  atomic_fetch_add_explicit =
      -lit("std::") >> "atomic_fetch_add_explicit" > '(' > '&' > atomic_memory_locations > ',' > expression > ',' > memory_order > ')';

  atomic_fetch_sub_explicit =
      -lit("std::") >> "atomic_fetch_sub_explicit" > '(' > '&' > atomic_memory_locations > ',' > expression > ',' > memory_order > ')';

  atomic_compare_exchange_weak_explicit =
      -lit("std::") >> "atomic_compare_exchange_weak_explicit" > '(' >
          '&' > atomic_memory_locations > ',' > int_ > ',' > int_ > ',' > memory_order > ',' > memory_order > ')';

  simple_expression =
    ( '(' > expression > ')' )
    | int_
    | bool_
    | register_locations
    | atomic_fetch_add_explicit
    | atomic_fetch_sub_explicit
    | atomic_compare_exchange_weak_explicit
    | load_statement
    | na_memory_locations;

  unary_expression =
      matches['!'] >> simple_expression;

  expression =
      unary_expression >> *(operators > expression);

  register_location =
      lexeme[ 'r' >> uint_ >> !(alnum | char_('_')) ];

  register_assignment =
      register_location >> '=' > expression;

  assignment =
      na_memory_locations >> '=' > expression;

  return_statement =
      "return" > expression;

  load_statement =
      atomic_memory_locations >> '.' >> "load" >> '(' >> (memory_order | attr(std::memory_order_seq_cst)) >> ')' >> -(".readsvalue(" >> int_ >> ')');

  store_statement =
      atomic_memory_locations >> '.' >> "store" >> '(' >> expression >> ((',' >> memory_order) | attr(std::memory_order_seq_cst)) >> ')';

  function_call =
      function_names >> '(' >> ')';

  break_statement =
      "break" >> dummy(false);

  wait_statement =
      condition_variables > '.' > "wait" > '(' > unique_locks > ',' > "[&]" > scope > ')';

  statement =
    ( ( register_assignment
      | assignment
      | return_statement
      | load_statement
      | store_statement
      | function_call
      | break_statement
      | atomic_fetch_add_explicit
      | atomic_fetch_sub_explicit
      | atomic_compare_exchange_weak_explicit
      | wait_statement
      ) > ';')
    | if_statement
    | while_statement;

  // Unused_type rules with on_success actions.
  scope_begin                 = (lit('{') - "{{{");
  scope_end                   = lit('}');
  threads_begin               = lit("{{{");
  threads_next                = lit("|||");
  threads_end                 = lit("}}}");

  // int main() { ... [return 0;] }   // the optional return statement is ignored.
  main =
      "int" > no_skip[whitespace] > string("main") > "()" > main_scope;

  main_scope =
      scope_begin > -body > scope_end;

  if_statement =
      lit("if") >> '(' >> expression >> ')' > body /*>> -("else" > body)*/;

  while_statement =
      lit("while") >> '(' >> expression >> ')' > body;

  // void function_name() { ... }
  function =
      "void" > no_skip[whitespace] > function_name > "()" > scope;

  function_name =
      identifier;

  scope =
      scope_begin > -body > scope_end;

  // The body of a function.
  body =
      +(declaration_statement | statement | scope | threads) >> dummy(false);

  threads =
      threads_begin > body >> +(threads_next > body) > threads_end >> dummy(false);

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
      (main_scope)
      (function)
      (function_name)
      (scope)
      (body)
      (threads)
      (cppmem)
      (statement)
      (simple_expression)
      (unary_expression)
      (expression)
      (register_assignment)
      (assignment)
      (load_statement)
      (store_statement)
      (register_location)
      (function_call)
      (if_statement)
      (while_statement)
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
template struct parser::grammar_cppmem<std::string::const_iterator>;
