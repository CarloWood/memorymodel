#pragma once

#include "grammar_whitespace.h"
#include "ast.h"
#include <stack>

template<typename Iterator>
struct position_handler;

namespace parser {

template<typename Iterator>
class grammar_statement : public qi::grammar<Iterator, ast::statement(), skipper<Iterator>>
{
 private:
  template<typename T> using rule = qi::rule<Iterator, T(), skipper<Iterator>>;

  grammar_whitespace<Iterator> whitespace;
  rule<ast::statement>         catchall;
  rule<ast::statement>         statement;
  rule<ast::assignment>        assignment;

 public:
  grammar_statement(position_handler<Iterator>& handler);
};

} // namespace parser
