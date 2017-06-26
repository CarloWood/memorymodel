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
  using namespace qi;

  unittest = (cppmem.type >> eoi)
           | (cppmem.memory_location >> eoi)
           | (cppmem.register_location >> eoi)
           | (cppmem.vardecl >> eoi)
           | (cppmem.main >> eoi)
           | (cppmem.function >> eoi)
           | (cppmem.compound_statement >> eoi)
           | (cppmem.threads >> eoi)
           | (cppmem.statement >> eoi)
           | cppmem;

  // Debugging and error handling and reporting support.
  using qi::debug;
  BOOST_SPIRIT_DEBUG_NODES(
      (unittest)
  );

  using position_handler_function = phoenix::function<position_handler<Iterator>>;

  // Error handling: on error in start, call handler.
  on_error<fail>
  (
      unittest
    , position_handler_function(handler)("Error! Expecting ", _4, _3)
  );

}

} // namespace parser

// Instantiate grammar template.
template class parser::grammar_unittest<std::string::const_iterator>;
