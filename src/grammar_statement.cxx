#include "sys.h"
#include "debug.h"
#include "grammar_cppmem.h"
#include "position_handler.h"
#include "SymbolsImpl.h"

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

namespace parser {

namespace phoenix = boost::phoenix;

//=====================================
// The statement grammar
//=====================================

template<typename Iterator>
grammar_statement<Iterator>::grammar_statement(position_handler<Iterator>& handler, grammar_cppmem<Iterator>& cppmem) :
    grammar_statement::base_type(statement, "grammar_statement")
{
  auto& na_memory_locations{Symbols::instance().m_impl->na_memory_locations};
  auto& atomic_memory_locations{Symbols::instance().m_impl->atomic_memory_locations};
  auto& register_locations{Symbols::instance().m_impl->register_locations};
  auto& function_names{Symbols::instance().m_impl->function_names};
  auto& if_statement{cppmem.if_statement};
  auto& while_statement{cppmem.while_statement};
  using namespace qi;

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
  ;

  atomic_fetch_add_explicit =
      -lit("std::") >> "atomic_fetch_add_explicit" >> '(' >> '&' >> atomic_memory_locations >> ',' >> expression >> ',' >> memory_order >> ')';

  load_statement =
      atomic_memory_locations >> '.' >> "load" >> '(' >> (memory_order | attr(std::memory_order_seq_cst)) >> ')' >> -(".readsvalue(" >> int_ >> ')');

  store_statement =
      atomic_memory_locations >> '.' >> "store" >> '(' >> expression >> ((',' >> memory_order) | attr(std::memory_order_seq_cst)) >> ')';

  simple_expression =
    ( '(' > expression > ')' )
    | int_
    | bool_
    | register_locations
    | atomic_fetch_add_explicit
    | load_statement
    | na_memory_locations;

  unary_expression =
      matches['!'] >> simple_expression;

  expression =
      unary_expression >> *(operators > expression);

  assignment =
      na_memory_locations >> '=' > expression;

  register_location =
      lexeme[ 'r' >> uint_ >> !(alnum | char_('_')) ];

  register_assignment =
      register_location >> '=' > expression;

  function_call =
      function_names >> '(' >> ')';

  statement =
    ( (register_assignment | assignment | load_statement | store_statement | function_call) > ';')
    | if_statement
    | while_statement;

  // Debugging and error handling and reporting support.
  using qi::debug;      // This macro uses plain 'debug', that is otherwise confused with our namespace of the same name.
  BOOST_SPIRIT_DEBUG_NODES(
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
  );

  using handler_function = phoenix::function<position_handler<Iterator>>;

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
}

} // namespace parser

// Instantiate grammar template.
template struct parser::grammar_statement<std::string::const_iterator>;
