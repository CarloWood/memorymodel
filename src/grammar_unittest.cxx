#include "sys.h"
#include "grammar_unittest.h"

namespace parser {

//=====================================
// Unit test grammar
//=====================================

template<typename Iterator>
grammar_unittest<Iterator>::grammar_unittest() :
    qi::grammar<Iterator, ast::nonterminal(), skipper<Iterator>>(unittest, "grammar_unittest"),
    cppmem("UNITTEST")
{
  unittest = cppmem.main
           | cppmem.vardecl
           | cppmem.function
           | cppmem.scope
           | cppmem.threads
           | cppmem.statement
           | cppmem.vardecl.type
           | cppmem.vardecl.memory_location
           | cppmem.vardecl.register_location;

  // Names of grammar rules.
  unittest.name("unittest");

  // Debugging and error handling and reporting support.
  BOOST_SPIRIT_DEBUG_NODES(
      (unittest)
  );
}

} // namespace parser

#include <boost/spirit/include/support_line_pos_iterator.hpp>

// Instantiate grammar templates.
template struct parser::grammar_unittest<std::string::const_iterator>;
