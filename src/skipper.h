#pragma once

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_confix.hpp>

namespace parser {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace repository = boost::spirit::repository;

//=====================================
// The skipper grammar
//=====================================
template<typename Iterator>
struct skipper : public qi::grammar<Iterator>
{
  skipper() : skipper::base_type(start, "skipper")
  {
    qi::eol_type eol;
    ascii::char_type char_;

    start =             // White space (tab/space/cr/lf) and/or comments.
            +ascii::space
          | c_comment
          | cpp_comment
          | if_0_endif;

    cpp_comment =       // C++-style comment.
            repository::confix("//", eol)[*(char_ - eol)];

    c_comment =         // C-style comment.
            repository::confix("/*", "*/")[*(char_ - "*/")];

    if_0_endif =        // Code commented out with #if 0 ... #endif.
            repository::confix("#if 0", "#endif")[*(char_ - "#endif")];
  }

  qi::rule<Iterator> cpp_comment;
  qi::rule<Iterator> c_comment;
  qi::rule<Iterator> if_0_endif;
  qi::rule<Iterator> start;
};

} // namespace parser
