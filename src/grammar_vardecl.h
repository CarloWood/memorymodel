#pragma once

#include "grammar_whitespace.h"
#include "ast.h"
#include "error_handler.h"

namespace parser {

template<typename Iterator>
class grammar_cppmem;

template<typename Iterator>
class grammar_unittest;

template<typename Iterator>
class grammar_vardecl : public qi::grammar<Iterator, ast::vardecl(), skipper<Iterator>>
{
 private:
  template<typename T> using rule = qi::rule<Iterator, T(), skipper<Iterator>>;
  using rule_noskip = qi::rule<Iterator, char()>;

  friend class grammar_cppmem<Iterator>;
  friend class grammar_unittest<Iterator>;

  grammar_whitespace<Iterator> whitespace;
  rule_noskip                  identifier_begin_char;
  rule_noskip                  identifier_char;
  rule<std::string>            identifier;
  rule<ast::type>              type;
  rule<ast::register_location> register_location;
  rule<ast::memory_location>   memory_location;
  rule<ast::vardecl>           vardecl;

 public:
  grammar_vardecl(error_handler<Iterator>& error_h);
};

} // namespace parser
