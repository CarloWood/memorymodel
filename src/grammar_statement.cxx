#include "sys.h"
#include "debug.h"
#include "grammar_statement.h"
#include "position_handler.h"
#include "Symbols.h"

BOOST_FUSION_ADAPT_STRUCT(ast::statement, m_statement)
BOOST_FUSION_ADAPT_STRUCT(ast::load_statement, m_memory_location_id, m_memory_order, m_readsvalue)
BOOST_FUSION_ADAPT_STRUCT(ast::store_statement, m_memory_location_id, m_val, m_memory_order)
BOOST_FUSION_ADAPT_STRUCT(ast::register_assignment, lhs, rhs)
BOOST_FUSION_ADAPT_STRUCT(ast::assignment, lhs, rhs)
BOOST_FUSION_ADAPT_STRUCT(ast::simple_expression, m_simple_expression_node)
BOOST_FUSION_ADAPT_STRUCT(ast::unary_expression, m_negated, m_simple_expression)
BOOST_FUSION_ADAPT_STRUCT(ast::chain, op, operand)
BOOST_FUSION_ADAPT_STRUCT(ast::expression, m_operand, m_chained)

namespace parser {

namespace phoenix = boost::phoenix;

//=====================================
// The statement grammar
//=====================================

template<typename Iterator>
grammar_statement<Iterator>::grammar_statement(position_handler<Iterator>& handler) :
    grammar_statement::base_type(statement, "grammar_statement"), vardecl(handler)
{
  auto& na_memory_locations(Symbols::instance().na_memory_locations);
  auto& atomic_memory_locations(Symbols::instance().atomic_memory_locations);
  auto& register_locations(Symbols::instance().register_locations);
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

  load_statement =
      atomic_memory_locations >> '.' >> "load" >> '(' >> (memory_order | attr(std::memory_order_seq_cst)) >> ')' >> -(".readsvalue(" >> int_ >> ')');

  store_statement =
      atomic_memory_locations >> '.' >> "store" >> '(' >> expression >> ((',' >> memory_order) | attr(std::memory_order_seq_cst)) >> ')';

  simple_expression =
    ( '(' > expression > ')' )
    | int_
    | register_locations
    | na_memory_locations
    | load_statement;

  unary_expression =
      matches['!'] >> simple_expression;

  expression =
      unary_expression >> *(operators > expression);

  assignment =
      na_memory_locations >> '=' > expression;

  register_assignment =
      vardecl.register_location >> '=' > expression;

  statement =
      (register_assignment | assignment | load_statement | store_statement) > ';';

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
