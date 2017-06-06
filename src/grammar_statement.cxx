#include "sys.h"
#include "debug.h"
#include "grammar_statement.h"
#include "position_handler.h"
#include "Symbols.h"

BOOST_FUSION_ADAPT_STRUCT(
  ast::statement,
  (ast::statement_node, m_statement)
)

BOOST_FUSION_ADAPT_STRUCT(
  ast::load_statement,
  (int, m_memory_location_id),
  (std::memory_order, m_memory_order),
  (boost::optional<int>, m_readsvalue)
)

BOOST_FUSION_ADAPT_STRUCT(
  ast::store_statement,
  (int, m_memory_location_id),
  (ast::expression, m_val),
  (std::memory_order, m_memory_order)
)

BOOST_FUSION_ADAPT_STRUCT(
  ast::register_assignment,
  (ast::register_location, lhs),
  (ast::expression, rhs)
)

BOOST_FUSION_ADAPT_STRUCT(
  ast::assignment,
  (int, lhs),
  (ast::expression, rhs)
)

namespace parser {

namespace phoenix = boost::phoenix;

struct new_register_handler {
  int operator()(ast::register_location& /*register_location*/)
  {
    return 0;
  }
};

//=====================================
// The statement grammar
//=====================================

template<typename Iterator>
grammar_statement<Iterator>::grammar_statement(position_handler<Iterator>& handler) :
    grammar_statement::base_type(statement, "grammar_statement"), vardecl(handler)
{
  qi::int_type int_;
  auto& na_memory_locations(Symbols::instance().na_memory_locations);
  auto& atomic_memory_locations(Symbols::instance().atomic_memory_locations);
  auto& register_locations(Symbols::instance().register_locations);

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

  load_statement =
      atomic_memory_locations >> '.' >> "load" >> '(' >> (memory_order | qi::attr(std::memory_order_seq_cst)) >> ')' >> -(".readsvalue(" >> int_ >> ')');

  store_statement =
      atomic_memory_locations >> '.' >> "store" >> '(' >> expression >> ((',' >> memory_order) | qi::attr(std::memory_order_seq_cst)) >> ')';

  expression =
      (int_ | na_memory_locations | load_statement);

  assignment =
      na_memory_locations >> '=' >> expression;

  register_assignment =
      vardecl.register_location >> '=' >> expression;

  register_assignment2 =
      register_assignment;

  statement =
      (register_assignment2 | assignment | load_statement | store_statement) > ';';

  // Debugging and error handling and reporting support.
  using qi::debug;
  BOOST_SPIRIT_DEBUG_NODES(
    (statement)
    (expression)
    (register_assignment)
    (assignment)
    (load_statement)
    (store_statement)
  );

  using qi::on_error;
  using qi::on_success;
  using qi::fail;
  using handler_function = phoenix::function<position_handler<Iterator>>;

  qi::_1_type _1;
  qi::_3_type _3;
  qi::_4_type _4;
  qi::_val_type _val;

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
