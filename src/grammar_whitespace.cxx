#include "sys.h"
#include "grammar_whitespace.h"

namespace parser {

//=====================================
// The whitespace grammar
//=====================================

template<typename Iterator>
grammar_whitespace<Iterator>::grammar_whitespace() :
    grammar_whitespace::base_type(whitespace, "grammar_whitespace")
{
  using namespace qi;
  using namespace ascii;
  using namespace repository;
  using qi::char_;
  using qi::space;

  // A C++ comment. Everything between '//' and eol.
  cpp_comment =
      confix("//", eol)  [ *(char_ - eol) ];

  // A C comment. Everything between /* and */.
  c_comment   =
      confix("/*", "*/") [ *(char_ - "*/") ];

  // White space. Tabs, spaces, new-lines, C-comments and C++-comments.
  whitespace  =
      +(+space | c_comment | cpp_comment);

  // Debugging and error handling and reporting support.
  using qi::debug;
  BOOST_SPIRIT_DEBUG_NODES(
      //(cpp_comment)
      //(c_comment)
      //(whitespace)
  );
}

} // namespace parser

// Instantiate grammar template.
template class parser::grammar_whitespace<std::string::const_iterator>;
