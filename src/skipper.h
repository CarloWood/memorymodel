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
    start =             // White space (tab/space/cr/lf) and/or comments.
            +ascii::space
          | c_comment
          | cpp_comment;

    cpp_comment =       // C++-style comment.
            repository::confix("//", qi::eol)[*(ascii::char_ - qi::eol)];

    c_comment =         // C-style comment.
            repository::confix("/*", "*/")[*(ascii::char_ - "*/")];
  }

  qi::rule<Iterator> cpp_comment;
  qi::rule<Iterator> c_comment;
  qi::rule<Iterator> start;
};

} // namespace parser
