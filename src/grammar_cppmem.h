#pragma once

#include "grammar_vardecl.h"
#include "error_handler.h"

namespace parser {

template <typename Iterator>
class grammar_unittest;

template <typename Iterator>
class grammar_cppmem : public qi::grammar<Iterator, ast::cppmem(), skipper<Iterator>>
{
 private:
  template<typename T> using rule = qi::rule<Iterator, T(), skipper<Iterator>>;
  using rule_noskip = qi::rule<Iterator, qi::unused_type()>;     // Rules appearing inside lexeme or no_skip etc, must not have a skipper.

  char const* const m_filename;

  // Needs access to the rules.
  friend class grammar_unittest<Iterator>;

  grammar_whitespace<Iterator>  whitespace;
  rule_noskip                   scope_begin;
  rule_noskip                   scope_end;
  rule_noskip                   threads_begin;
  rule_noskip                   threads_next;
  rule_noskip                   threads_end;

  rule<ast::statement>          catchall;

  grammar_vardecl<Iterator>     vardecl;
  rule<ast::statement>          statement;
  rule<qi::unused_type>         assignment;
  rule<ast::body>               body;
  rule<ast::scope>              scope;
  rule<ast::function_name>      function_name;
  rule<ast::function>           function;

  rule<ast::function>           main;
  rule<ast::scope>              main_scope;
  rule<qi::unused_type>         return_statement;

  rule<ast::threads>            threads;
  rule<ast::cppmem>             cppmem;

 public:
  grammar_cppmem(char const* filename, error_handler<Iterator>& error_handler);
};

} // namespace parser
