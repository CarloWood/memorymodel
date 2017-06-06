#pragma once

#include "grammar_whitespace.h"
#include "grammar_vardecl.h"
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

  grammar_whitespace<Iterator>                  whitespace;
  grammar_vardecl<Iterator>                     vardecl;
  rule<ast::expression>                         expression;
  rule<ast::statement>                          statement;
  rule<ast::register_assignment>                register_assignment;
  rule<ast::assignment>                         register_assignment2;
  rule<ast::assignment>                         assignment;
  qi::symbols<char, std::memory_order>          memory_order;
  rule<ast::load_statement>                     load_statement;
  rule<ast::store_statement>                    store_statement;

 public:
  grammar_statement(position_handler<Iterator>& handler);
};

} // namespace parser
