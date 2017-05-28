#include "sys.h"
#include "grammar_unittest.h"

namespace parser {

namespace phoenix = boost::phoenix;

//=====================================
// Unit test grammar
//=====================================

template<typename Iterator>
grammar_unittest<Iterator>::grammar_unittest(position_handler<Iterator>& handler) :
    grammar_unittest::base_type(unittest, "grammar_unittest"),
    cppmem(handler)
{
  qi::eoi_type eoi;

  unittest = (cppmem.vardecl.type >> eoi)
           | (cppmem.vardecl.memory_location >> eoi)
           | (cppmem.vardecl.register_location >> eoi)
           | (cppmem.vardecl >> eoi)
           | cppmem.main
           | cppmem.function
           | cppmem.scope
           | cppmem.threads
           | cppmem.statement;

  // Names of grammar rules.
  unittest.name("unittest");

  // Debugging and error handling and reporting support.
  BOOST_SPIRIT_DEBUG_NODES(
      (unittest)
  );

  using qi::on_error;
  using qi::fail;
  using position_handler_function = phoenix::function<position_handler<Iterator>>;

  qi::_3_type _3;
  qi::_4_type _4;

  // Error handling: on error in start, call handler.
  on_error<fail>
  (
      unittest
    , position_handler_function(handler)("Error! Expecting ", _4, _3)
  );

}

} // namespace parser

// Instantiate grammar template.
template struct parser::grammar_unittest<std::string::const_iterator>;
