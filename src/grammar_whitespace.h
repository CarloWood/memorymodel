#pragma once

#include "grammar_general.h"

namespace parser {

template<typename Iterator>
class grammar_whitespace : public qi::grammar<Iterator, qi::unused_type()>
{
 private:
  using rule_noskip = qi::rule<Iterator, qi::unused_type()>;

  rule_noskip cpp_comment;
  rule_noskip c_comment;
  rule_noskip whitespace;

 public:
  grammar_whitespace();
};

} // namespace parser
