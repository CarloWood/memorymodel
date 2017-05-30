#include "sys.h"
#include "grammar_statement.h"
#include "position_handler.h"

namespace parser {

namespace phoenix = boost::phoenix;

//=====================================
// The statement grammar
//=====================================

template<typename Iterator>
grammar_statement<Iterator>::grammar_statement(position_handler<Iterator>& handler) :
    grammar_statement::base_type(statement, "grammar_statement")
{
  qi::char_type char_;

  // Statements.
  catchall                    = !(char_('{') | char_('|') | "return") >> +(char_ - (char_('}') | ';')) >> ';';
  statement                   = /*assignment |*/ catchall;
  //assignment                  = m_symbols >> '=' >> int_ >> ';';

  // Names of grammar rules.
  statement.name("statement");

  // Debugging and error handling and reporting support.
  BOOST_SPIRIT_DEBUG_NODES(
      (catchall)
      (statement)
  );

  using qi::on_error;
  using qi::fail;
  using handler_function = phoenix::function<position_handler<Iterator>>;

  qi::_3_type _3;
  qi::_4_type _4;

  // Error handling: on error in start, call handler.
  on_error<fail>
  (
      statement
    , handler_function(handler)("Error! Expecting ", _4, _3)
  );
}

} // namespace parser

// Instantiate grammar template.
template struct parser::grammar_statement<std::string::const_iterator>;
