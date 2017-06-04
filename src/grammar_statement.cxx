#include "sys.h"
#include "grammar_statement.h"
#include "position_handler.h"
#include "Symbols.h"

BOOST_FUSION_ADAPT_STRUCT(
  ast::expression,
  (int, v)
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
  using namespace qi::labels;
  auto& memory_locations(Symbols::instance().memory_locations);
  auto& register_locations(Symbols::instance().register_locations);

  statement =
      (register_assignment2 | assignment);

  expression =
      int_;

  register_assignment2 =
      register_assignment;

  register_assignment =
      vardecl.register_location >> '=' >> expression >> ';';

  assignment =
      memory_locations >> '=' >> expression >> ';';

  // Debugging and error handling and reporting support.
  using qi::debug;
  BOOST_SPIRIT_DEBUG_NODES(
    (statement)
    (assignment)
    (memory_locations)
    (register_locations)
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
