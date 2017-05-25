#include "sys.h"
#include "grammar_cppmem.h"

BOOST_FUSION_ADAPT_STRUCT(
    ast::function,
    (ast::function_name, m_function_name),
    (ast::scope, m_scope)
)

BOOST_FUSION_ADAPT_STRUCT(
    ast::body,
    (ast::body::container_type, m_body_nodes),
    (bool, m_dummy)
)

BOOST_FUSION_ADAPT_STRUCT(
    ast::scope,
    (boost::optional<ast::body>, m_body)
)

BOOST_FUSION_ADAPT_STRUCT(
    ast::threads,
    (ast::threads::container_type, m_threads),
    (bool, m_dummy)
)

namespace parser {

//=====================================
// Cppmem program grammar
//=====================================

template<typename Iterator>
grammar_cppmem<Iterator>::grammar_cppmem(char const* filename) :
    qi::grammar<Iterator, ast::cppmem(), skipper<Iterator>>(cppmem, "grammar_cppmem"),
    m_filename(filename)
{
  using ascii::char_;

  // Unused_type rules with semantic actions.
  scope_begin                 = (qi::lit('{') - "{{{");
  scope_end                   = qi::lit('}');
  threads_begin               = qi::lit("{{{");
  threads_next                = qi::lit("|||");
  threads_end                 = qi::lit("}}}");

  // int main() { ... [return 0;] }   // the optional return statement is ignored.
  main                        = "int" >> qi::no_skip[whitespace] >> qi::string("main") >> "()" >> main_scope;
  main_scope                  = scope_begin >> -body >> -return_statement >> scope_end;
  return_statement            = "return" >> qi::no_skip[whitespace] >> qi::int_ >> ';';

  // void function_name() { ... }
  function                    = ("void" >> qi::no_skip[whitespace] >> function_name >> "()" >> scope);
  function_name               = vardecl.identifier;
  scope                       = scope_begin >> -body > scope_end;

  // The body of a function.
  body                        = +(vardecl | statement | scope | threads) >> qi::attr(false);
  threads                     = threads_begin > body >> +(threads_next > body) > threads_end >> qi::attr(false);

  // Statements.
  catchall                    = !(char_('{') | char_('|') | "return") >> +(char_ - (char_('}') | ';')) >> ';';
  statement                   = /*assignment |*/ catchall;
  //assignment                  = m_symbols >> '=' >> qi::int_ >> ';';

  cppmem                      = *(vardecl | function) > main;

  // Names of grammar rules.
  scope_begin.name("scope_begin");
  scope_end.name("scope_end");
  threads_begin.name("threads_begin");
  threads_next.name("threads_next");
  threads_end.name("threads_end");
  main.name("main");
  main_scope.name("main_scope");
  return_statement.name("return_statement");
  function.name("function");
  function_name.name("function_name");
  scope.name("scope");
  body.name("body");
  threads.name("threads");
  statement.name("statement");
  assignment.name("assignment");
  catchall.name("catchall");
  cppmem.name("cppmem");

  // Debugging and error handling and reporting support.
  BOOST_SPIRIT_DEBUG_NODES(
      (scope_begin)
      (scope_end)
      (threads_begin)
      (threads_next)
      (threads_end)
      (main)
      (main_scope)
      (return_statement)
      (function)
      (function_name)
      (scope)
      (body)
      (threads)
      (statement)
      (assignment)
      (catchall)
      (cppmem)
  );
}

} // namespace parser

#include <boost/spirit/include/support_line_pos_iterator.hpp>

// Instantiate grammar templates.
template struct parser::grammar_cppmem<boost::spirit::line_pos_iterator<std::string::const_iterator>>;
template struct parser::grammar_cppmem<std::string::const_iterator>;
