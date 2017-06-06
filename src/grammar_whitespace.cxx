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
  qi::eol_type eol;
  qi::lit_type lit;
  ascii::char_type char_;
  ascii::space_type space;
  repository::confix_type confix;

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
      (cpp_comment)
      (c_comment)
      (whitespace)
  );
}

} // namespace parser

// Instantiate grammar template.
template struct parser::grammar_whitespace<std::string::const_iterator>;
