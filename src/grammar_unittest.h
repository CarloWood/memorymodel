#pragma once

#include "grammar_cppmem.h"

namespace parser {

template <typename Iterator>
class grammar_unittest : public qi::grammar<Iterator, ast::nonterminal(), skipper<Iterator>>
{
 private:
  template<typename T> using rule = qi::rule<Iterator, T(), skipper<Iterator>>;

  grammar_cppmem<Iterator> cppmem;
  rule<ast::nonterminal>   unittest;

 public:
  grammar_unittest(position_handler<Iterator>& handler);
};

} // namespace parser
